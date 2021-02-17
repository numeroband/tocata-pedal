#pragma once

#define HAL_PICO

#include "switch_matrix.pio.h"
#include "ws2812b.pio.h"
#include "config.h"

extern "C" {
#include <hardware/i2c.h>
#include <hardware/gpio.h>
#include <hardware/flash.h>
#include <hardware/watchdog.h>
#include <pico/bootrom.h>
#include <pico/unique_id.h>
}

#include <pico/stdio.h>
#include <pico/stdio/driver.h>
#include <pico/time.h>

#include <tusb.h>
#include <cstdint>

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

static constexpr uint32_t kFlashPageSize = FLASH_PAGE_SIZE;
static constexpr uint32_t kFlashPartitionOffset = 0x1c0000;
static constexpr uint32_t kFlashSize = 0x200000;

static inline void flash_read(uint32_t flash_offs, void *dst, size_t count)
{
    const uint8_t* partition = reinterpret_cast<const uint8_t*>(XIP_BASE);
    memcpy(dst, partition + flash_offs, count);
}

static inline void flash_write(uint32_t flash_offs, const void *data, size_t count)
{
    flash_range_program(flash_offs, static_cast<const uint8_t*>(data), count);
}

static inline void flash_erase(uint32_t flash_offs, size_t count)
{
    flash_range_erase(flash_offs, count);
}

// Switches

static inline void switches_init(const HWConfigSwitches& config)
{
    uint offset = pio_add_program(pio0, &switch_matrix_program);
    switch_matrix_program_init(pio0, config.state_machine_id, offset, config.first_input_pin, config.first_output_pin);
    printf("Configured switch_matrix program in sm %u offset %u\n", config.state_machine_id, offset);
}

static inline bool switches_changed(const HWConfigSwitches& config)
{
    return !pio_sm_is_rx_fifo_empty(pio0, config.state_machine_id);
}

static inline uint32_t switches_value(const HWConfigSwitches& config)
{
    return ~pio_sm_get(pio0, config.state_machine_id) >> 4;
}

// Leds

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