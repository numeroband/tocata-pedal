#pragma once

#include <chrono>
#include <thread>
#include <cstring>
#include <cassert>

namespace tocata {

// System
static inline uint32_t millis() { return uint32_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()); }
static inline void sleep_ms(uint32_t ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
static inline void idle_loop() { sleep_ms(1); }
static inline void board_program() {}

void board_reset();

// Flash

static constexpr uint32_t kFlashAddress = 0x10000000;
static constexpr uint32_t kFlashSectorSize = 4 * 1024;
static constexpr uint32_t kFlashPageSize = 256;
static constexpr uint32_t kFlashPartitionOffset = 512 * 1024;
static constexpr uint32_t kFlashPartitionSize = 128 * 1024;
static constexpr uint32_t kFlashSize = 2 * 1024 * 1024;

void flash_read(uint32_t flash_offs, void *dst, size_t count);

void flash_write(uint32_t flash_offs, const void *data, size_t count);

void flash_erase(uint32_t flash_offs, size_t count);

// Switches

static inline void switches_init(const HWConfigSwitches& config) {}

bool switches_changed(const HWConfigSwitches& config);

uint32_t switches_value(const HWConfigSwitches& config);

// Expression

static inline void expression_init(const HWConfigExpression& config) {}
uint16_t expression_read();

// Leds

static inline void leds_init(const HWConfigLeds& config) {}
static inline bool leds_fix_mapping() { return true; }

void leds_refresh(const HWConfigLeds& config, uint32_t* leds, size_t num_leds);

// I2C

static inline void i2c_init(uint32_t baudrate, const HWConfigI2C& config) {}
void i2c_write(uint8_t index, uint8_t addr, const uint8_t *src, size_t len);

//SPI
static inline void spi_init() {}
void spi_transfer(const uint8_t *src, size_t len);
void set_dc(bool enabled);

// BOARD LED

static inline void board_led_init() {}
static inline void board_led_enable(bool enabled) {}

// USB

void usb_init();
void usb_run();

// USB web
uint32_t usb_vendor_available();
uint32_t usb_vendor_read(void* buffer, uint32_t bufsize);
uint32_t usb_vendor_write_available();
uint32_t usb_vendor_write(const void* buffer, uint32_t bufsize);
uint32_t usb_vendor_write_flush();

// USB midi
size_t usb_midi_write(const unsigned char* message, size_t size);
static inline void usb_midi_write(uint8_t val1, uint8_t val2) { 
    uint8_t msg[] = {val1, val2};
    usb_midi_write(msg, sizeof(msg));
} 
static inline void usb_midi_write(uint8_t val1, uint8_t val2, uint8_t val3) {
    uint8_t msg[] = {val1, val2, val3};
    usb_midi_write(msg, sizeof(msg));
}

//
bool is_pedal_long();

}
