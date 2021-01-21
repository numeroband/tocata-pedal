#pragma once

#include <esp_partition.h>

namespace tocata {

static constexpr const char* FILE_WRITE = "w";
static constexpr const char* FILE_READ = "r";

class FS
{
private:
    class Block;

public:
    class File {
    public:
        size_t read(void* dst, size_t size) { return _fs ? _fs->read(*this, dst, size) : 0; }
        size_t write(const void* src, size_t size) { return _fs ? _fs->write(*this, src, size) : 0; }
        bool isEmpty() const { return isEmpty(_flags); }
        void close() { _fs = nullptr; }
        void id() { idFromFlags(_flags); }
        operator bool() const { return _fs; }

    protected:
        friend class FS;
        friend class Block;

        static uint8_t idFromFlags(uint8_t flags) { return (flags & kIdMask) - 1; }
        static uint8_t emptyFlags(uint8_t file_id) { return (file_id + 1) | kEmptyMask; }
        static bool isFree(uint8_t flags) { return flags == kFree; }
        static bool isInvalid(uint8_t flags) { return flags == kInvalid; }
        static bool isEmpty(uint8_t flags) { return flags & kEmptyMask; }
        static bool isAvailable(uint8_t flags) { return isFree(flags) || isInvalid(flags); }

        File() : _fs(nullptr) {}
        File(FS* fs, uint8_t block_id, uint8_t index, uint8_t flags) :
            _fs(fs), _block_id(block_id), _index(index), _flags(flags) {}
        uint8_t blockId() const { return _block_id; }
        size_t index() const { return _index; }
        void invalidate() { _flags = kInvalid; }
        void updateNotEmpty() { _flags &= ~kEmptyMask; }
        uint8_t flags() { return _flags; }

    private:
        static constexpr uint8_t kFree = 0xFF;
        static constexpr uint8_t kEmptyMask = 0x80;
        static constexpr uint8_t kIdMask = 0x7F;
        static constexpr uint8_t kInvalid = 0x00;

        FS* _fs;
        uint8_t _block_id;
        uint8_t _index;
        uint8_t _flags;
    };

    FS(size_t file_size = 512, size_t block_size = 64 * 1024) : 
        _block(this, file_size, block_size) {}
    bool begin(bool formatOnFail = false);
    File open(const char* path, const char* mode = FILE_READ);
    void remove(const char* path);
    bool exists(const char* path) { return open(path, FILE_READ); }
    size_t usedBytes() const { return _used_bytes; }
    size_t totalBytes() const { return (_num_blocks - 1) * _block.totalBytes(); }
    
protected:
    size_t read(File& file, void* dst, size_t size);
    size_t write(File& file, const void* src, size_t size);

private:
    class Block 
    {
    public:
        static constexpr uint8_t kInvalidId = 0xFF;

        Block(FS* fs, size_t file_size, size_t block_size);
        void begin(const esp_partition_t* partition) { _partition = partition; }
        void load(uint8_t id);
        void erase() { erase(_id); }
        void erase(uint8_t id);
        FS::File open(uint8_t file_id);
        FS::File createFile(uint8_t file_id);
        void invalidateFile(FS::File& file);
        void compactInto(uint8_t block_id);
        size_t read(File& file, void* dst, size_t size);
        size_t write(File& file, const void* src, size_t size);
        size_t size() const { return _block_size; }
        bool isValid() const { return _desc->header.magic == Descriptor::kMagic; }
        bool isEmpty() const { return _desc->flags[0] == File::kFree; }
        bool hasSpace(uint8_t block_id) const;
        bool hasSpace() const { return _desc->flags[filesPerBlock() - 1] == File::kFree; }
        size_t bytesPerFile() const { return _file_size - 1; }
        size_t usedBytes() const;
        size_t totalBytes() const { return filesPerBlock() * bytesPerFile(); }
        uint8_t id() const { return _id; }
        uint8_t cycles() const { return _desc->header.cycles; }
        bool canReuse() const;

    private:
        struct Descriptor
        {
            static constexpr uint32_t kMagic = 0x70CA7AF5;
            struct Header
            {
                uint32_t magic;
                uint32_t cycles;
            };
            Header header;
            uint8_t flags[];
        };

        void updateFlags(uint8_t index, uint8_t flag);
        size_t filesPerBlock() const { return (_block_size / _file_size) - 1; }
        size_t offset() const { return _id * _block_size; }
        size_t descSize() const { return sizeof(Descriptor) + filesPerBlock(); }
        size_t fileOffset(uint8_t index) { return offset() + (_file_size * (index + 1)); }
        size_t fileContentOffset(uint8_t index) { return fileOffset(index) + sizeof(uint8_t); }
        size_t indexOffset(uint8_t index) { return offset() + sizeof(_desc->header) + index; }

        FS* _fs;
        size_t _file_size;
        size_t _block_size;
        Descriptor* _desc;
        uint8_t _id = 0xFF;
        const esp_partition_t* _partition;
    };

    File create(uint8_t file_id);

    Block _block;
    size_t _used_bytes;
    size_t _num_blocks;
    size_t _empty_blocks;
    uint8_t _extra_block_id;
};

extern FS TocataFS;

}