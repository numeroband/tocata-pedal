#pragma once

#define HAL_PICO

#include "switches.pio.h"
#include "ws2812b.pio.h"
#include "config.h"

extern "C" {
#include <hardware/i2c.h>
#include <hardware/gpio.h>
#include <hardware/flash.h>
#include <hardware/watchdog.h>
#include <hardware/adc.h>
#include <pico/bootrom.h>
#include <pico/unique_id.h>
}

#include <pico/stdio.h>
#include <pico/stdio/driver.h>
#include <pico/time.h>

#include <tusb.h>
#include <cstdint>

namespace tocata {

// System

static inline void idle_loop() {}
static inline uint32_t millis() { return to_ms_since_boot(get_absolute_time()); }
static inline void sleep_ms(uint32_t ms) { ::sleep_ms(ms); }
static inline void board_reset() { watchdog_enable(50, 0); }
static inline void board_program() 
{
  add_alarm_in_ms(50, [](alarm_id_t, void *) {
    reset_usb_boot(0, 1);
    return 0LL;
  }, NULL, false);
}


// Flash
#define MEMFLASH 0

static constexpr uint32_t kFlashSectorSize = FLASH_SECTOR_SIZE;
static constexpr uint32_t kFlashPageSize = FLASH_PAGE_SIZE;
static constexpr uint32_t kFlashAddress = XIP_BASE;

#if MEMFLASH
static constexpr uint32_t kFlashPartitionSize = 2 * 64 * 1024;
static constexpr uint32_t kFlashPartitionOffset = 0;
static constexpr uint32_t kFlashSize = kFlashPartitionSize;
extern uint8_t MemFlash[];
#else
static constexpr uint32_t kFlashPartitionSize = 4 * 64 * 1024;
static constexpr uint32_t kFlashPartitionOffset = 512 * 1024;
static constexpr uint32_t kFlashSize = 2 * 1024 * 1024;
#endif

static inline void flash_init()
{
#if MEMFLASH
memset(MemFlash, 0xFF, kFlashSize);
#endif
}

static inline void flash_read(uint32_t flash_offs, void *dst, size_t count)
{
    assert(flash_offs + count <= kFlashSize);
#if MEMFLASH
    printf("flash_read 0x%08X %u bytes\n", flash_offs, (uint32_t)count);
    memcpy(dst, MemFlash + flash_offs, count);
#else
    const uint8_t* flash = reinterpret_cast<const uint8_t*>(XIP_BASE);
    memcpy(dst, flash + flash_offs, count);
#endif
}

static inline void flash_write(uint32_t flash_offs, const void *data, size_t count)
{
    assert(flash_offs + count <= kFlashSize);
    assert(flash_offs % kFlashPageSize == 0);
    assert(count % kFlashPageSize == 0);
#if MEMFLASH
    printf("flash_write 0x%08X %u bytes\n", flash_offs, (uint32_t)count);
    for (size_t i = 0; i < count; ++i)
    {
        MemFlash[flash_offs + i] &= static_cast<const uint8_t*>(data)[i];
    }
#else
    flash_range_program(flash_offs, static_cast<const uint8_t*>(data), count);
#endif
}

static inline void flash_erase(uint32_t flash_offs, size_t count)
{
    assert(flash_offs + count <= kFlashSize);
    assert(flash_offs % kFlashSectorSize == 0);
    assert(count % kFlashSectorSize == 0);
#if MEMFLASH
    printf("flash_erase 0x%08X %u bytes\n", flash_offs, (uint32_t)count);
    memset(MemFlash + flash_offs, 0xFF, count);
#else
    flash_range_erase(flash_offs, count);
#endif
}

// Switches

static inline void switches_init(const HWConfigSwitches& config)
{
    uint offset = pio_add_program(pio0, &switches_program);
    switches_program_init(pio0, config.state_machine_id, offset, config.first_input_pin, config.first_output_pin);
    printf("Configured switches program in sm %u offset %u\n", config.state_machine_id, offset);
}

static inline bool switches_changed(const HWConfigSwitches& config)
{
    return !pio_sm_is_rx_fifo_empty(pio0, config.state_machine_id);
}

static inline uint32_t switches_value(const HWConfigSwitches& config)
{
    return ~pio_sm_get(pio0, config.state_machine_id);
}

// Expression

static inline void expression_init(const HWConfigExpression& config) 
{
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(config.adc_pin);
    // Select ADC input (starts at GPIO26)
    adc_select_input(config.adc_pin - 26);
}

static inline uint16_t expression_read()
{
    return adc_read();
}

// Leds

bool leds_fix_mapping();

static inline void leds_init(const HWConfigLeds& config)
{
    uint offset = pio_add_program(pio0, &ws2812b_program);
    ws2812b_program_init(pio0, config.state_machine_id, offset, config.data_pin);
    printf("Configured ws2812b program in sm %u offset %u\n", config.state_machine_id, offset);
}

static inline void leds_refresh(const HWConfigLeds& config, uint32_t* leds, size_t num_leds)
{
    printf("Leds update sm %u:", config.state_machine_id);
    for (size_t led = 0; led < num_leds; ++led)
    {
        const uint32_t led_value = leds[led];
        printf(" %08X", led_value);
        pio_sm_put_blocking(pio0, config.state_machine_id, led_value);
    }    
    printf("\n");
}

// I2C

static inline void i2c_init(uint32_t baudrate, const HWConfigI2C& config)
{
	uint actual_baudrate = ::i2c_init(i2c0, baudrate);
	gpio_set_function(config.sda_pin, GPIO_FUNC_I2C);
	gpio_set_function(config.scl_pin, GPIO_FUNC_I2C);
	gpio_pull_up(config.sda_pin);
	gpio_pull_up(config.scl_pin);
    printf("i2c initialized to %u Hz\n", actual_baudrate);
}

static inline void i2c_write(uint8_t addr, const uint8_t *src, size_t len)
{
    int ret = i2c_write_timeout_per_char_us(i2c0, addr, src, len, false, 100);
    if (ret != len)
    {
        printf("I2C error %d with %u bytes to %02X: ", ret, (uint32_t)len, addr);
        for (uint8_t i = 0; i < len; ++i)
        {
            printf("%02X ", src[i]);
        }
        printf("\n");
    }
}

// BOARD LED

static inline void board_led_init()
{
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, 1);
}

static inline void board_led_enable(bool enabled)
{
    gpio_put(PICO_DEFAULT_LED_PIN, enabled);
}

// USB

void usb_init();

static inline void usb_run() { tud_task(); }
static inline uint32_t usb_vendor_available() { return tud_vendor_available(); }
static inline uint32_t usb_vendor_read(void* buffer, uint32_t bufsize) { return tud_vendor_read(buffer, bufsize); }
static inline uint32_t usb_vendor_write_available() { return tud_vendor_write_available(); }
static inline uint32_t usb_vendor_write(const void* buffer, uint32_t bufsize) { return tud_vendor_write(buffer, bufsize); }

static inline void usb_midi_write(uint8_t val1, uint8_t val2) 
{ 
  uint8_t message[] = { val1, val2 };
  tud_midi_write(0, message, sizeof(message));
}

static inline void usb_midi_write(uint8_t val1, uint8_t val2, uint8_t val3) 
{ 
  uint8_t message[] = { val1, val2, val3 };
  tud_midi_write(0, message, sizeof(message));
}

}