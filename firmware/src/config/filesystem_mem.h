#pragma once

#include "flash_partition.h"

#include <cstdio>
#include <cstring>
#include <cassert>

namespace tocata {

static constexpr const char* FILE_WRITE = "w";
static constexpr const char* FILE_READ = "r";

class File {
public:
    size_t read(void* dst, size_t size)
    {
        assert(size <= sizeof(content()->bytes));
        // printf("read file %u bytes from %u\n", (uint32_t)size, _fd);

        if (!content() || content()->state == kNoFile)
        {
            return 0;
        }

        if (content()->state == kEmpty)
        {
            memset(dst, 0, size);
        }
        else
        {
            memcpy(dst, content()->bytes, size);
        }

        return size;
    }

    size_t write(const void* src, size_t size)
    {
        assert(size <= sizeof(content()->bytes));
        // printf("write file %u bytes to %u\n", (uint32_t)size, _fd);

        if (!content() || content()->state == kNoFile || size == 0)
        {
            return 0;
        }
        content()->state = kUsed;
        memcpy(content()->bytes, src, size);

        return size;
    }

    bool isEmpty() const
    {
        return content()->state != kUsed;
    }

    void close() 
    { 
        // printf("close file %u\n", _fd);
        _fd = kInvalidFile;
    }

    operator bool() const { return content() && content()->state != kNoFile; }

    // ~File()
    // {
    //     if (_fd != kInvalidFile)
    //     {
    //         printf("destroyed file %u without closing\n", _fd);
    //     }
    // }

protected:
    friend class FS;

    static size_t maxNumFiles() { return kNumFiles; }
    static size_t maxFileSize() { return kFileSize; }

    File(uint8_t fd, bool write = false) : _fd(fd)
    {
        if (write)
        {
            content()->state = kEmpty; 
        }
        // printf("open file %u to %s\n", _fd, write ? "write" : "read");
    }

    static void remove(uint8_t fd)
    {
        // printf("remove file %u\n", fd);
        files[fd].state = kNoFile;
    }

    size_t size() { return (content() && content()->state == kUsed) ? sizeof(content()->bytes) : 0; }

private:
    static constexpr uint8_t kInvalidFile = 0xFF;
    static constexpr size_t kNumFiles = 100;
    static constexpr size_t kFileSize = 511;

    enum State
    {
        kNoFile,
        kEmpty,
        kUsed,        
    };

    struct Content
    {
        uint8_t state;
        uint8_t bytes[kFileSize];
    };

    static File::Content files[kNumFiles];

    Content* content() { return _fd == kInvalidFile ? nullptr : &files[_fd]; }
    const Content* content() const { return _fd == kInvalidFile ? nullptr : &files[_fd]; }

    uint8_t _fd;
};

class FS
{
public:
    bool init(bool formatOnFail = false) { return true; }
    File open(const char* path, const char* mode = FILE_READ)
    {
        assert(mode && mode[1] == '\0' && (mode[0] == 'r' || mode[0] == 'w'));
        bool write = (mode[0] == 'w');
        return {fileId(path), write};
    }

    void remove(const char* path)
    {
        File::remove(fileId(path));
    }

    bool exists(const char* path) { return open(path); }

    size_t usedBytes() const 
    { 
        size_t bytes = 0;
        for (uint8_t i = 0; i < File::maxNumFiles(); ++i)
        {
            File file{i};
            bytes += file.size();
        }
        return bytes; 
    }

    size_t totalBytes() const { return File::maxNumFiles() * File::maxFileSize(); }

private:
    static uint8_t fileId(const char* path)
    {
        static auto isNumber = [](char c) { return c >= '0' && c <= '9'; };
        assert(path && path[0] == '/' && path[3] == '\0' && isNumber(path[1]) && isNumber(path[2]));
        return (path[1] - '0') * 10 + (path[2] - '0');
    }
};

extern FS TocataFS;

}