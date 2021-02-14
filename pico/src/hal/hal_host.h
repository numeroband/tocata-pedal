#pragma once

#include <chrono>
#include <thread>
#include <cstring>
#include <cassert>

namespace tocata {

struct HWConfigI2C
{
    uint8_t sda_pin;
    uint8_t scl_pin;		
};

struct HWConfigSwitches
{
    int state_machine_id;
    uint8_t first_input_pin;
    uint8_t first_output_pin;
};

struct HWConfigLeds
{
    int state_machine_id;
    uint8_t data_pin;
};

// System
static inline uint32_t millis() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static inline void sleep_ms(uint32_t ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
static inline void idle_loop() { sleep_ms(1); }
static inline void board_reset() {}
static inline void board_program() {}

// Flash

static constexpr uint32_t kFlashPageSize = 4 * 1024;
static constexpr uint32_t kFlashPartitionOffset = 0x1c0000;
static constexpr uint32_t kFlashSize = 0x200000;

extern uint8_t MemFlash[kFlashSize];

static inline void flash_read(uint32_t flash_offs, void *dst, size_t count) 
{
    memcpy(dst, MemFlash + flash_offs, count);
}

static inline void flash_write(uint32_t flash_offs, const void *data, size_t count) 
{
    for (size_t i = 0; i < count; ++i)
    {
        MemFlash[flash_offs + i] &= static_cast<const uint8_t*>(data)[i];
    }
}

static inline void flash_erase(uint32_t flash_offs, size_t count) 
{
    assert(flash_offs % kFlashPageSize == 0);
    assert(count % kFlashPageSize == 0);
    memset(MemFlash + flash_offs, 0xFF, count);
}

// Switches

static inline void switches_init(const HWConfigSwitches& config) {}

static inline bool switches_changed(const HWConfigSwitches& config)
{
    return false;
}

static inline uint32_t switches_value(const HWConfigSwitches& config)
{
    return 0;
}

// Leds

static inline void leds_init(const HWConfigLeds& config) {}

static inline void leds_refresh(const HWConfigLeds& config, uint32_t* leds, size_t num_leds) {}
// I2C

static inline void i2c_init(uint32_t baudrate, const HWConfigI2C& config) {}
void i2c_write(uint8_t addr, const uint8_t *src, size_t len);

// BOARD LED

static inline void board_led_init() {}
static inline void board_led_enable(bool enabled) {}

// USB

void usb_init();
void usb_run();

uint32_t usb_vendor_available();
uint32_t usb_vendor_read(void* buffer, uint32_t bufsize);
uint32_t usb_vendor_write_available();
uint32_t usb_vendor_write(const void* buffer, uint32_t bufsize);

static inline void usb_midi_write(uint8_t val1, uint8_t val2) {} 
static inline void usb_midi_write(uint8_t val1, uint8_t val2, uint8_t val3) {}

}