#pragma once

#define MEMFS 0
#if MEMFS
#include "filesystem_mem.h"
#else

#include "flash_partition.h"

namespace tocata {

static constexpr const char* FILE_WRITE = "w";
static constexpr const char* FILE_READ = "r";

class FS;

class File {
public:
    size_t read(void* dst, size_t size);
    size_t write(const void* src, size_t size);
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


class FS
{
private:
    class Block;

public:
    FS() : _block(this) {}
    bool init(bool formatOnFail = false);
    void test();
    void printUsage();
    File open(const char* path, const char* mode = FILE_READ);
    void remove(const char* path);
    bool exists(const char* path) { return open(path, FILE_READ); }
    size_t usedBytes() const { return _used_bytes; }
    size_t totalBytes() const { return (_block.numBlocks() - 1) * _block.totalBytes(); }
    
protected:
    friend class File;
    size_t read(File& file, void* dst, size_t size);
    size_t write(File& file, const void* src, size_t size);

private:
    class Block 
    {
    public:
        static constexpr uint8_t kInvalidId = 0xFF;
        static constexpr size_t kFileSize = 512;
        static constexpr size_t kBlockSize = 64 * 1024;
        static constexpr size_t kMaxBlocks = 10;
        static constexpr size_t kFilesPerBlock = (kBlockSize / kFileSize) - 1;

        static constexpr size_t size() { return kBlockSize; }
        static constexpr size_t bytesPerFile() { return kFileSize - 1; }

        Block(FS* fs) : _fs(fs) {}
        bool init(const FlashPartition* partition, bool formatOnFail);
        void load(uint8_t id);
        void erase() { erase(_id); }
        void erase(uint8_t id);
        File open(uint8_t file_id);
        File createFile(uint8_t file_id);
        void invalidateFile(File& file);
        void compactInto(uint8_t block_id);
        size_t read(File& file, void* dst, size_t size);
        size_t write(File& file, const void* src, size_t size);
        bool isEmpty() const { return _cached_flags[0] == File::kFree; }
        bool hasSpace(uint8_t block_id) const;
        bool hasSpace() const { return _cached_flags[kFilesPerBlock - 1] == File::kFree; }
        size_t usedBytes() const;
        size_t totalBytes() const { return kFilesPerBlock * bytesPerFile(); }
        uint8_t id() const { return _id; }
        uint8_t cycles() const { return cycles(_id); }
        uint8_t cycles(uint8_t id) const { return _cycles[id]; }
        bool canReuse() const;
        uint8_t numBlocks() const { return _partition->size() / kBlockSize; }

    private:
        struct Descriptor
        {
            struct Header
            {
                static constexpr uint32_t kMagic = 0x70CA7AF5;
                uint32_t magic;
                uint32_t cycles;
                bool isValid() const { return magic == kMagic; }
                Header() : magic(kMagic) {}
            };
            Header header;
            uint8_t flags[Block::kFilesPerBlock];
        };

        static size_t offset(uint8_t id) { return id * kBlockSize; }
        static size_t fileOffset(uint8_t id, uint8_t index) { return offset(id) + (kFileSize * (index + 1)); }
        static size_t fileContentOffset(uint8_t id, uint8_t index) { return fileOffset(id, index) + sizeof(uint8_t); }
        static size_t indexOffset(uint8_t id, uint8_t index) { return offset(id) + sizeof(((Descriptor*)0)->header) + index; }

        void updateFlags(uint8_t index, uint8_t flag);
        size_t offset() const { return offset(_id); }
        size_t fileOffset(uint8_t index) const { return fileOffset(_id, index); }
        size_t fileContentOffset(uint8_t index) const { return fileContentOffset(_id, index); }
        size_t indexOffset(uint8_t index) const { return indexOffset(_id, index); }

        FS* _fs;
        uint8_t _cached_flags[Block::kFilesPerBlock];
        uint32_t _cycles[Block::kMaxBlocks] = {};
        const FlashPartition* _partition = nullptr;
        uint8_t _id = kInvalidId;
    };

    File create(uint8_t file_id);

    Block _block;
    size_t _used_bytes;
    uint8_t _extra_block_id;
    FlashPartition _partition{};
};

extern FS TocataFS;

}
#endif // MEMFS