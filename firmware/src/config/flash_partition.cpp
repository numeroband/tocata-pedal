#include "flash_partition.h"
#include "hal.h"
#include <cstring>

namespace tocata {

FlashPartition::FlashPartition() : _part_offset(kFlashPartitionOffset) {}

bool FlashPartition::read(size_t src_offset, void* dst_buf, size_t dst_size) const
{
    flash_read(_part_offset + src_offset, dst_buf, dst_size);
    return true;
}

size_t FlashPartition::writePage(size_t dst_offset, const void* src_buf, size_t src_size) const
{
    // This function cannot be called multiple times so make it static
    static uint8_t buf[kFlashPageSize];

    // Page aligned offset and size
    uint32_t aligned_offset = dst_offset & ~(kFlashPageSize - 1);
    uint32_t prologue = dst_offset - aligned_offset;
    uint32_t epilogue = 0;    
    if (prologue + src_size < kFlashPageSize)
    {
        epilogue = kFlashPageSize - (prologue + src_size);
    }
    else
    {
        src_size = kFlashPageSize - prologue;
    }

    if (prologue || epilogue)
    {
        memset(buf, 0xFF, prologue);
        memcpy(buf + prologue, src_buf, src_size);
        memset(buf + prologue + src_size, 0xFF, epilogue);
        src_buf = buf;
        dst_offset = aligned_offset;
    }

    flash_write(_part_offset + dst_offset, static_cast<const uint8_t*>(src_buf), kFlashPageSize);
    return src_size;
}

bool FlashPartition::write(size_t dst_offset, const void* src_buf, size_t src_size) const
{
    const uint8_t* buf = static_cast<const uint8_t*>(src_buf);
    while(src_size)
    {
        size_t written = writePage(dst_offset, buf, src_size);
        dst_offset += written;
        buf += written;
        src_size -= written;
    }

    return true;
}

bool FlashPartition::erase(size_t offset, size_t size) const
{
    flash_erase(_part_offset + offset, size);
    return true;
}

size_t FlashPartition::size() const
{
    return kFlashPartitionSize;
}


}