#include "filesystem.h"
#include <Arduino.h>

namespace tocata {

FS TocataFS{};

bool FS::begin(bool formatOnFail)
{
    auto partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, nullptr);
    assert(partition);

    _block.begin(partition);
    _num_blocks = partition->size / _block.size();
    _empty_blocks = 0;
    _used_bytes = 0;

    uint32_t empty_block_cycles = UINT32_MAX;

    // Read backwards to finish with block 0
    for (uint8_t i = _num_blocks; i > 0; --i)
    {
        _block.load(i - 1);
        if (!_block.isValid())
        {
            if (formatOnFail)
            {
                _block.erase();
            }
            else
            {
                return false;
            }
        }
        if (_block.isEmpty())
        {
            if (_block.cycles() < empty_block_cycles)
            {
                _extra_block_id = _block.id();
                empty_block_cycles = _block.cycles();
            }
            ++_empty_blocks;
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
        for (uint8_t id = 0; id < _num_blocks; ++id)
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
    uint8_t available_block = Block::kInvalidId;
    for (uint8_t id = 0; id < _num_blocks; ++id)
    {
        if (id == _extra_block_id)
        {
            continue;
        }

        if (_block.hasSpace(id))
        {
            available_block = id;
            break;
        }
    }

    if (available_block != Block::kInvalidId)
    {
        _block.load(available_block);
        return _block.createFile(file_id);
    }

    uint32_t min_cycles = UINT32_MAX;
    for (uint8_t id = 0; id < _num_blocks; ++id)
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
        _used_bytes -= _block.bytesPerFile();
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

FS::Block::Block(FS* fs, size_t file_size, size_t block_size) : 
            _fs(fs), _file_size(file_size), _block_size(block_size),
            _desc(reinterpret_cast<Descriptor*>(new uint8_t[descSize()])),
            _id(kInvalidId)
{
}

void FS::Block::load(uint8_t id)
{
    if (_id != id)
    {
        _id = id;
        auto ret = esp_partition_read(_partition, offset(), _desc, descSize());
        assert(ret == ESP_OK);
    }
}

void FS::Block::erase(uint8_t id)
{
    if (_id != id)
    {
        _id = id;
        auto ret = esp_partition_read(_partition, offset(), &_desc->header, sizeof(_desc->header));
        assert(ret == ESP_OK);
    }

    if (isValid())
    {
        ++_desc->header.cycles;
    }
    else
    {
        _desc->header.cycles = 1;
        _desc->header.magic = Descriptor::kMagic;    
    }

    auto ret = esp_partition_erase_range(_partition, offset(), size());
    assert(ret == ESP_OK);
    ret = esp_partition_write(_partition, offset(), &_desc->header, sizeof(_desc->header));
    assert(ret == ESP_OK);

    memset(_desc->flags, 0xFF, filesPerBlock());

    Serial.print("Erased block ");
    Serial.print(_id);
    Serial.print(" cycles ");
    Serial.println(_desc->header.cycles);
}

FS::File FS::Block::open(uint8_t file_id)
{
    for (uint8_t i = 0; i < filesPerBlock(); ++i)
    {
        uint8_t flags = _desc->flags[i];
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
    for (uint8_t i = 0; i < filesPerBlock(); ++i)
    {
        uint8_t flags = _desc->flags[i];
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
    _desc->flags[index] = flags;
    auto ret = esp_partition_write(_partition, indexOffset(index), &flags, sizeof(flags));
    assert(ret == ESP_OK); 
    ret = esp_partition_write(_partition, fileOffset(index), &flags, sizeof(flags));
    assert(ret == ESP_OK);
    // Serial.printf("update block %u index %u flags %02X\n", _id, index, flags);
}

void FS::Block::compactInto(uint8_t dst_block_id)
{
    size_t dst_offset = dst_block_id * _block_size;
    size_t dst_flags_off = dst_offset + sizeof(_desc->header);
    size_t dst_file_off = dst_offset + _file_size;

    uint8_t file_content[_file_size];
    for (uint8_t i = 0; i < filesPerBlock(); ++i)
    {
        uint8_t flags = _desc->flags[i];
        if (File::isAvailable(flags))
        {
            continue;
        }

        auto ret = esp_partition_write(_partition, dst_flags_off++, &flags, sizeof(flags));
        assert(ret == ESP_OK);
        if (!File::isEmpty(flags))
        {
            ret = esp_partition_read(_partition, fileContentOffset(i), file_content, _file_size);
            assert(ret == ESP_OK);
            ret = esp_partition_write(_partition, dst_file_off, file_content, _file_size);
            assert(ret == ESP_OK);
        }
        dst_file_off += _file_size;
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
    size_t block_offset = block_id * _block_size;
    size_t flags_offset = block_offset + sizeof(_desc->header);
    size_t last_flags_offset = flags_offset + (filesPerBlock() - 1);
    uint8_t flags;
    auto ret = esp_partition_read(_partition, last_flags_offset, &flags, sizeof(flags));
    assert(ret == ESP_OK);

    return File::isEmpty(flags);    
}

size_t FS::Block::usedBytes() const
{
    uint8_t files_used = 0;
    for (uint8_t i = 0; i < filesPerBlock(); ++i)
    {
        uint8_t flags = _desc->flags[i];
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
    for (uint8_t i = 0; i < filesPerBlock(); ++i)
    {
        if (File::isAvailable(_desc->flags[i]))
        {
            return true;
        }
    }
    return false;
}

}