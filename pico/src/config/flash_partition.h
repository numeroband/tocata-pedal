#pragma once

#include <cstdio>
#include <cstdint>

namespace tocata {

class FlashPartition {
public:
    FlashPartition();
    bool read(size_t src_offset, void* dst_buf, size_t dst_size) const;
    bool write(size_t dst_offset, const void* src_buf, size_t src_size) const;
    bool erase(size_t offset, size_t size) const;
    size_t size() const;

private:
    static uint32_t flashOffset(size_t offset);
    uint32_t _part_offset = 0;
};

}