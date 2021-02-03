#include "filesystem.h"
#include <cstring>
#include <assert.h>

#define _log(...)
#define _logln(...)
#define _logf(...)

namespace tocata {

FS TocataFS{};

size_t File::read(void* dst, size_t size) 
{ 
    return _fs ? _fs->read(*this, dst, size) : 0; 
}

size_t File::write(const void* src, size_t size) { 
    return _fs ? _fs->write(*this, src, size) : 0; 
}

bool FS::begin(bool formatOnFail)
{
    if (!_block.begin(&_partition, formatOnFail))
    {
        return false;
    }

    _used_bytes = 0;

    uint32_t extra_block_cycles = UINT32_MAX;
    // Read backwards to finish with block 0
    for (uint8_t i = _block.numBlocks(); i > 0; --i)
    {
        _block.load(i - 1);
        if (_block.isEmpty())
        {
            if (_block.cycles() < extra_block_cycles)
            {
                _extra_block_id = _block.id();
                extra_block_cycles = _block.cycles();
            }
        }
        else
        {
            _used_bytes += _block.usedBytes();
        }        
    }
    if (_extra_block_id == 0)
    {
        _block.load(1);
    }

    _log("Extra block ");
    _logln(_extra_block_id);

    return true;
}

File FS::open(const char* path, const char* mode)
{
    bool write = (mode[0] == 'w');
    uint8_t file_id = (path[1] - '0') * 10 + (path[2] - '0');
    uint8_t first_block = _block.id();
    File file = _block.open(file_id);
    if (!file)
    {
        for (uint8_t id = 0; id < _block.numBlocks(); ++id)
        {
            if (id == _extra_block_id || id == first_block)
            {
                continue;
            }

            _block.load(id);
            file = _block.open(file_id);
            if (file)
            {
                break;
            }
        }
    }

    if (!write)
    {
        return file;
    }

    bool existed = file;
    if (existed)
    {
        if (file.isEmpty())
        {
            return file;
        }

        _block.invalidateFile(file);
    }

    file = create(file_id);
    if (!existed && file)
    {
        _used_bytes += _block.bytesPerFile();
    }

    return file;
}

File FS::create(uint8_t file_id)
{
    uint32_t min_cycles = UINT32_MAX;
    uint8_t available_block = Block::kInvalidId;
    for (uint8_t id = 0; id < _block.numBlocks(); ++id)
    {
        if (id == _extra_block_id)
        {
            continue;
        }

        uint32_t cycles = _block.cycles(id);
        if (_block.hasSpace(id) && cycles < min_cycles)
        {
            available_block = id;
            min_cycles = cycles;
        }
    }

    if (available_block != Block::kInvalidId)
    {
        _block.load(available_block);
        return _block.createFile(file_id);
    }

    min_cycles = UINT32_MAX;
    for (uint8_t id = 0; id < _block.numBlocks(); ++id)
    {
        if (id == _extra_block_id)
        {
            continue;
        }

        _block.load(id);
        if (_block.cycles() < min_cycles && _block.canReuse())
        {
            min_cycles = _block.cycles();
            available_block = _block.id();
        }
    }

    if (available_block == Block::kInvalidId)
    {
        // Filesystem full
        return {};
    }

    _block.load(available_block);
    _block.compactInto(_extra_block_id);
    _block.erase();
    _block.load(_extra_block_id);
    _extra_block_id = available_block;

    return _block.createFile(file_id);
}

void FS::remove(const char* path)
{
    auto file = open(path, FILE_READ);
    if (!file)
    {
        return;
    }

    _block.load(file.blockId());
    _block.invalidateFile(file);
    _used_bytes -= _block.bytesPerFile();
}

size_t FS::read(File& file, void* dst, size_t size)
{
    _block.load(file.blockId());
    return _block.read(file, dst, size);
}

size_t FS::write(File& file, const void* src, size_t size)
{
    _block.load(file.blockId());
    return _block.write(file, src, size);
}

bool FS::Block::begin(const FlashPartition* partition, bool formatOnFail)
{
    _partition = partition;

    Descriptor::Header header;
    for (uint8_t id = 0; id < numBlocks(); ++id)
    {
        auto ret = _partition->read(offset(id), &header, sizeof(header));
        assert(ret);

        if (header.isValid())
        {
            _cycles[id] = header.cycles;
            _log("Found block ");
            _log(id);
            _log(" cycles ");
            _logln(cycles(id));
        }
        else
        {
            if (!formatOnFail)
            {
                return false;
            }
            erase(id);
        }
    }

    return true;
}

void FS::Block::load(uint8_t id)
{
    if (_id != id)
    {
        _id = id;
        auto ret = _partition->read(indexOffset(0), &_cached_flags, sizeof(_cached_flags));
        assert(ret);
    }
}

void FS::Block::erase(uint8_t id)
{
    _id = id;
    Descriptor::Header header;
    header.cycles = ++_cycles[id];

    auto ret = _partition->erase(offset(), size());
    assert(ret);
    ret = _partition->write(offset(), &header, sizeof(header));
    assert(ret);

    memset(_cached_flags, 0xFF, kFilesPerBlock);

    _log("Erased block ");
    _log(_id);
    _log(" cycles ");
    _logln(cycles());
}

File FS::Block::open(uint8_t file_id)
{
    for (uint8_t i = 0; i < kFilesPerBlock; ++i)
    {
        uint8_t flags = _cached_flags[i];
        if (File::isFree(flags))
        {
            break;
        }

        if (File::idFromFlags(flags) == file_id)
        {
            return {_fs, _id, i, flags};
        }
    }
    return {};
}

File FS::Block::createFile(uint8_t file_id)
{
    for (uint8_t i = 0; i < kFilesPerBlock; ++i)
    {
        uint8_t flags = _cached_flags[i];
        if (File::isFree(flags))
        {
            flags = File::emptyFlags(file_id);
            updateFlags(i, flags);
            return {_fs, _id, i, flags};
        }
    }
    _log("Cannot create file ");
    _log(file_id);
    _log(" in block ");
    _logln(_id);
    return {};
}

void FS::Block::invalidateFile(File& file) 
{
    file.invalidate();
    updateFlags(file.index(), file.flags()); 
}

void FS::Block::updateFlags(uint8_t index, uint8_t flags)
{
    _cached_flags[index] = flags;
    auto ret = _partition->write(indexOffset(index), &flags, sizeof(flags));
    assert(ret); 
    ret = _partition->write(fileOffset(index), &flags, sizeof(flags));
    assert(ret);
    _logf("update block %u index %u flags %02X\n", _id, index, flags);
}

void FS::Block::compactInto(uint8_t dst_block_id)
{
    size_t dst_offset = dst_block_id * kBlockSize;
    size_t dst_flags_off = dst_offset + offsetof(Descriptor, flags);
    size_t dst_file_off = dst_offset + kFileSize;

    uint8_t file_content[kFileSize];
    for (uint8_t i = 0; i < kFilesPerBlock; ++i)
    {
        uint8_t flags = _cached_flags[i];
        if (File::isAvailable(flags))
        {
            continue;
        }

        auto ret = _partition->write(dst_flags_off++, &flags, sizeof(flags));
        assert(ret);
        if (!File::isEmpty(flags))
        {
            ret = _partition->read(fileContentOffset(i), file_content, kFileSize);
            assert(ret);
            ret = _partition->write(dst_file_off, file_content, kFileSize);
            assert(ret);
        }
        dst_file_off += kFileSize;
    }
}   

size_t FS::Block::read(File& file, void* dst, size_t size)
{
    if (size > bytesPerFile())
    {
        return 0;
    }

    if (file.isEmpty())
    {
        memset(dst, 0, size);
    }
    else
    {
        auto ret = _partition->read(fileContentOffset(file.index()), dst, size);
        assert(ret);
    }

    return size;
}

size_t FS::Block::write(File& file, const void* src, size_t size)
{
    if (size > bytesPerFile())
    {
        return 0;
    }

    if (file.isEmpty())
    {
        file.updateNotEmpty();
        updateFlags(file.index(), file.flags());
    }

    auto ret = _partition->write(fileContentOffset(file.index()), src, size);
    assert(ret);

    return size;
}

bool FS::Block::hasSpace(uint8_t block_id) const
{
    size_t last_flags_offset = indexOffset(block_id, kFilesPerBlock - 1);
    uint8_t flags;
    auto ret = _partition->read(last_flags_offset, &flags, sizeof(flags));
    assert(ret);

    return File::isFree(flags);
}

size_t FS::Block::usedBytes() const
{
    uint8_t files_used = 0;
    for (uint8_t i = 0; i < kFilesPerBlock; ++i)
    {
        uint8_t flags = _cached_flags[i];
        if (File::isFree(flags))
        {
            break;
        }

        if (!File::isInvalid(flags))
        {
            ++files_used;
        }
    }

    return files_used * bytesPerFile();
}


bool FS::Block::canReuse() const
{
    for (uint8_t i = 0; i < kFilesPerBlock; ++i)
    {
        if (File::isAvailable(_cached_flags[i]))
        {
            return true;
        }
    }
    return false;
}

}