#include "flash_partition.h"
#include "hal.h"
#include <cstring>

#define FAKE_FLASH 0

namespace {
    static constexpr uint32_t kFlashSize = 0x200000;
    static constexpr uint32_t kPartitionOffset = 0x1c0000;
}

namespace tocata {

#if FAKE_FLASH
static uint8_t FakeFlash[128 * 1024];
#endif

FlashPartition::FlashPartition() : _part_offset(kPartitionOffset) 
{
    erase(0, size());
}

bool FlashPartition::read(size_t src_offset, void* dst_buf, size_t dst_size) const
{
#if FAKE_FLASH
    // printf("read(%u, 0x%08X, %u)\n", src_offset, (uint32_t)dst_buf, dst_size);
    memcpy(dst_buf, FakeFlash + src_offset, dst_size);
#else
    flash_read(_part_offset + src_offset, dst_buf, dst_size);
#endif

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

#if FAKE_FLASH
    // printf("write(%u, 0x%08X, %u)\n", dst_offset, (uint32_t)src_buf, src_size);
    for (uint32_t i = 0; i < src_size; ++i)
    {
        FakeFlash[dst_offset + i] &= static_cast<const uint8_t*>(src_buf)[i];
    }
#else
    flash_write(_part_offset + dst_offset, static_cast<const uint8_t*>(src_buf), src_size);
#endif

    return true;
}

bool FlashPartition::erase(size_t offset, size_t size) const
{
#if FAKE_FLASH
    // printf("erase(%u, %u)\n", offset, size);
    memset(FakeFlash + offset, 0xFF, size);
#else
    flash_erase(_part_offset + offset, size);
#endif
    return true;
}

size_t FlashPartition::size() const
{
#if FAKE_FLASH
    return sizeof(FakeFlash); 
#else
    return kFlashSize - _part_offset;
#endif
}


}