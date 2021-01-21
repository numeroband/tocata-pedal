#include "filesystem.h"
#include <Arduino.h>

namespace tocata {

FS TocataFS{};

bool FS::begin(bool formatOnFail)
{
    auto partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, nullptr);
    assert(partition);

    _block.begin(partition);
    _used_bytes = 0;

    for (uint8_t i = 0; i < _block.numBlocks(); ++i)
    {
        if (!_block.updateCycles(i) && !formatOnFail)
        {
            return false;
        }
    }

    for (uint8_t i = 0; i < _block.numBlocks(); ++i)
    {
        _block.init(i);
    }

    _block.printCycles();

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

    Serial.print("Extra block ");
    Serial.println(_extra_block_id);

    return true;
}

FS::File FS::open(const char* path, const char* mode)
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

FS::File FS::create(uint8_t file_id)
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

void FS::Block::init(uint8_t id)
{
    Descriptor::Header header;
    auto ret = esp_partition_read(_partition, offset(id), &header.magic, sizeof(header.magic));
    assert(ret == ESP_OK);

    if (!header.isValid())
    {
        Serial.print("Initializing block ");
        Serial.print(id);
        Serial.print(" with header ");
        Serial.println(header.magic);
        erase(id);
    }
}

void FS::Block::load(uint8_t id)
{
    if (_id != id)
    {
        _id = id;
        auto ret = esp_partition_read(_partition, indexOffset(0), &_desc.flags, sizeof(_desc.flags));
        assert(ret == ESP_OK);
    }
}

void FS::Block::erase(uint8_t id)
{
    _id = id;
    ++_desc.header.cycles[_id];
    ++_total_cycles;

    auto ret = esp_partition_erase_range(_partition, offset(), size());
    assert(ret == ESP_OK);
    ret = esp_partition_write(_partition, offset(), &_desc.header, sizeof(_desc.header));
    assert(ret == ESP_OK);

    memset(_desc.flags, 0xFF, kFilesPerBlock);

    Serial.print("Erased block ");
    Serial.print(_id);
    Serial.print(" cycles ");
    Serial.println(cycles());
}

FS::File FS::Block::open(uint8_t file_id)
{
    for (uint8_t i = 0; i < kFilesPerBlock; ++i)
    {
        uint8_t flags = _desc.flags[i];
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

FS::File FS::Block::createFile(uint8_t file_id)
{
    for (uint8_t i = 0; i < kFilesPerBlock; ++i)
    {
        uint8_t flags = _desc.flags[i];
        if (File::isFree(flags))
        {
            flags = File::emptyFlags(file_id);
            updateFlags(i, flags);
            return {_fs, _id, i, flags};
        }
    }
    return {};
}

void FS::Block::invalidateFile(FS::File& file) 
{
    file.invalidate();
    updateFlags(file.index(), file.flags()); 
}

void FS::Block::updateFlags(uint8_t index, uint8_t flags)
{
    _desc.flags[index] = flags;
    auto ret = esp_partition_write(_partition, indexOffset(index), &flags, sizeof(flags));
    assert(ret == ESP_OK); 
    ret = esp_partition_write(_partition, fileOffset(index), &flags, sizeof(flags));
    assert(ret == ESP_OK);
    Serial.printf("update block %u index %u flags %02X\n", _id, index, flags);
}

void FS::Block::compactInto(uint8_t dst_block_id)
{
    size_t dst_offset = dst_block_id * kBlockSize;
    size_t dst_flags_off = dst_offset + offsetof(Descriptor, flags);
    size_t dst_file_off = dst_offset + kFileSize;

    uint8_t file_content[kFileSize];
    for (uint8_t i = 0; i < kFilesPerBlock; ++i)
    {
        uint8_t flags = _desc.flags[i];
        if (File::isAvailable(flags))
        {
            continue;
        }

        auto ret = esp_partition_write(_partition, dst_flags_off++, &flags, sizeof(flags));
        assert(ret == ESP_OK);
        if (!File::isEmpty(flags))
        {
            ret = esp_partition_read(_partition, fileContentOffset(i), file_content, kFileSize);
            assert(ret == ESP_OK);
            ret = esp_partition_write(_partition, dst_file_off, file_content, kFileSize);
            assert(ret == ESP_OK);
        }
        dst_file_off += kFileSize;
    }
}   

size_t FS::Block::read(FS::File& file, void* dst, size_t size)
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
        auto ret = esp_partition_read(_partition, fileContentOffset(file.index()), dst, size);
        assert(ret == ESP_OK);
    }

    return size;
}

size_t FS::Block::write(FS::File& file, const void* src, size_t size)
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

    auto ret = esp_partition_write(_partition, fileContentOffset(file.index()), src, size);
    assert(ret == ESP_OK);

    return size;
}

bool FS::Block::hasSpace(uint8_t block_id) const
{
    size_t block_offset = block_id * kBlockSize;
    size_t flags_offset = block_offset + offsetof(Descriptor, flags);
    size_t last_flags_offset = flags_offset + (kFilesPerBlock - 1);
    uint8_t flags;
    auto ret = esp_partition_read(_partition, last_flags_offset, &flags, sizeof(flags));
    assert(ret == ESP_OK);

    return File::isEmpty(flags);    
}

size_t FS::Block::usedBytes() const
{
    uint8_t files_used = 0;
    for (uint8_t i = 0; i < kFilesPerBlock; ++i)
    {
        uint8_t flags = _desc.flags[i];
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
        if (File::isAvailable(_desc.flags[i]))
        {
            return true;
        }
    }
    return false;
}

bool FS::Block::updateCycles(uint8_t id)
{
    Descriptor::Header header;
    auto ret = esp_partition_read(_partition, offset(id), &header, sizeof(header));
    assert(ret == ESP_OK);

    if (!header.isValid())
    {
        Serial.print("Invalid block ");
        Serial.print(id);
        Serial.print(" with header ");
        Serial.println(header.magic);
        return false;
    }

    if (header.num_blocks != _desc.header.num_blocks)
    {
        Serial.print("Invalidating block ");
        Serial.print(id);
        Serial.print(" with num blocks ");
        Serial.print(header.num_blocks);
        Serial.print(" instead of ");
        Serial.println(_desc.header.num_blocks);
        header.magic = 0;
        auto ret = esp_partition_write(_partition, offset(id), &header.magic, sizeof(header.magic));
        assert(ret == ESP_OK);
        return false;
    }

    size_t total_cycles = 0;
    for (uint8_t i = 0; i < _desc.header.num_blocks; ++i)
    {
        size_t cycles = header.cycles[i];
        total_cycles += cycles;
    }

    if (total_cycles > _total_cycles)
    {
        _desc.header = header;
        _total_cycles = total_cycles;
    }

    return true;
}

void FS::Block::printCycles()
{
    for (uint8_t i = 0; i < _desc.header.num_blocks; ++i)
    {
        Serial.print("Block: ");
        Serial.print(i);
        Serial.print(" cycles: ");
        Serial.println(cycles(i));
    }
    Serial.print("Total cycles: ");
    Serial.println(_total_cycles);
}

}