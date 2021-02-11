#pragma once

#include <chrono>
#include <unistd.h>

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

// System
using namespace std::chrono;
static inline uint32_t millis() { return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();; }
static inline void sleep_ms(uint32_t ms) { usleep(ms * 1000); }
static inline void board_reset() {}
static inline void board_program() {}

// Flash

static constexpr uint32_t kFlashPageSize = 4 * 1024;

static inline void flash_read(uint32_t flash_offs, void *dst, size_t count) {}
static inline void flash_write(uint32_t flash_offs, const void *data, size_t count) {}
static inline void flash_erase(uint32_t flash_offs, size_t count) {}

// PIO

static inline void switches_init(const HWConfigSwitches& config) {}

static inline bool switches_changed(const HWConfigSwitches& config)
{
    return false;
}

static inline uint32_t switches_value(const HWConfigSwitches& config)
{
    return 0;
}

// I2C

static inline void i2c_init(uint32_t baudrate, const HWConfigI2C& config) {}
static inline void i2c_write(uint8_t addr, const uint8_t *src, size_t len) {}

// BOARD LED

static inline void board_led_init() {}
static inline void board_led_enable(bool enabled) {}

// USB

static inline void usb_init() {}
static inline void usb_run() {}
static inline uint32_t usb_vendor_available() { return false; }
static inline uint32_t usb_vendor_read(void* buffer, uint32_t bufsize) { return 0; }
static inline uint32_t usb_vendor_write_available() { return 0; }
static inline uint32_t usb_vendor_write(const void* buffer, uint32_t bufsize) { return 0; }
static inline void usb_midi_write(uint8_t val1, uint8_t val2) {} 
static inline void usb_midi_write(uint8_t val1, uint8_t val2, uint8_t val3) {}

}