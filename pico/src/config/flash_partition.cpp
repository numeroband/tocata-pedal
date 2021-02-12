#include "flash_partition.h"
#include "hal.h"
#include <cstring>

namespace tocata {

FlashPartition::FlashPartition() : _part_offset(kFlashPartitionOffset) 
{
    erase(0, size());
}

bool FlashPartition::read(size_t src_offset, void* dst_buf, size_t dst_size) const
{
    flash_read(_part_offset + src_offset, dst_buf, dst_size);
    return true;
}

bool FlashPartition::write(size_t dst_offset, const void* src_buf, size_t src_size) const
{
    // This function cannot be called multiple times so make it static
    static uint8_t buf[kFlashPageSize];

    // Page aligned offset and size
    const uint32_t offset = dst_offset & ~(kFlashPageSize - 1);
    const uint32_t prologue = dst_offset - offset;
    const uint32_t size = (prologue + src_size + kFlashPageSize - 1) & ~(kFlashPageSize - 1);
    const uint32_t epilogue = size - (prologue + src_size);

    if (prologue || epilogue)
    {
        memset(buf, 0xFF, prologue);
        memcpy(buf + prologue, src_buf, src_size);
        memset(buf + prologue + src_size, 0xFF, epilogue);
        src_buf = buf;
        dst_offset = offset;
        src_size = size;
    }

    flash_write(_part_offset + dst_offset, static_cast<const uint8_t*>(src_buf), src_size);
    return true;
}

bool FlashPartition::erase(size_t offset, size_t size) const
{
    flash_erase(_part_offset + offset, size);
    return true;
}

size_t FlashPartition::size() const
{
    return kFlashSize - _part_offset;
}


}