#include "hal.h"
#ifndef HAL_PICO

#include "display_sim_sh1106.h"
#include "display_sim_ssd1322.h"
#include "application.h"

#include <midi_sysex.h>
#include <config.h>
#include <libremidi/libremidi.hpp>

#include <functional>
#include <deque>
#include <mutex>
#include <vector>
#include <cstdlib>

namespace tocata {

uint8_t MemFlash[2 * 1024 * 1024];

static libremidi::midi_out midi{};
static DisplaySimSSD1322 display_ssd1322{};
static DisplaySimSH1106 display_sh1106{};
static DisplaySim& display = is_pedal_long() ? (DisplaySim&)display_ssd1322 : (DisplaySim&)display_sh1106;
static Application app{display};

void i2c_write(uint8_t addr, const uint8_t *src, size_t len)
{
    if (display.processTransfer(src, uint32_t(len)))
    {
      return;
    }

    printf("Invalid [%02X] %u bytes to %02X: ", addr, (uint32_t)len, addr);
    for (uint8_t i = 0; i < len; ++i)
    {
    	printf("%02X ", src[i]);
    }
    printf("\n");
}

void spi_transfer(const uint8_t *src, size_t len)
{
    if (display.processTransfer(src, uint32_t(len)))
    {
      return;
    }

    printf("Invalid %u bytes: ", (uint32_t)len);
    for (uint8_t i = 0; i < len; ++i)
    {
    	printf("%02X ", src[i]);
    }
    printf("\n");
}

void spi_set_dc(bool enabled)
{
  display.setControlData(enabled);
}

void spi_set_reset(bool enabled) {
}

void spi_set_cs(bool enabled) {
}

void leds_refresh(const HWConfigLeds& config, uint32_t* leds, size_t num_leds)
{
  auto adjust = [](uint32_t v32) -> uint8_t {
    uint8_t v8 = static_cast<uint8_t>(v32);
    return (v8 == 0) ? 0 : (v8 + 127);
  };

  for (uint8_t i = 0; i < num_leds; ++i) {
    uint32_t led = leds[i];
    uint8_t r = adjust(led >> 16);
    uint8_t g = adjust(led >> 24);
    uint8_t b = adjust(led >> 8);
    app.setLedColor(i, r, g, b);
  }
}

uint32_t switches_value(const HWConfigSwitches& config) 
{
  return app.switchesValue();
}

static FILE* flash;
constexpr const char* kFlashPath = "/tmp/tocata_flash";

void flash_init() 
{
  flash = fopen(kFlashPath, "a");
  assert(flash);  
  fclose(flash);
  flash = fopen(kFlashPath, "r+");
  assert(flash);  
  fseek(flash, kFlashSize - 1, SEEK_SET);
  int c = fgetc(flash);
  c = (c == EOF) ? 0xFF : c;
  fseek(flash, kFlashSize - 1, SEEK_SET);
  int new_c = fputc(c, flash);  
  assert(c == new_c);
  fflush(flash);
}

void flash_read(uint32_t flash_offs, void *dst, size_t count) 
{
  assert(flash_offs + count <= kFlashSize);
  fseek(flash, flash_offs, SEEK_SET);
  size_t ret = fread(dst, count, 1, flash);
  assert(ret == 1);
    // printf("flash_read 0x%08X %u bytes\n", flash_offs, (uint32_t)count);
    // memcpy(dst, MemFlash + flash_offs, count);
}

void flash_write(uint32_t flash_offs, const void *data, size_t count) 
{
    assert(flash_offs + count <= kFlashSize);
    assert(flash_offs % kFlashPageSize == 0);
    assert(count % kFlashPageSize == 0);
    uint8_t* buffer = new uint8_t[count];
    flash_read(flash_offs, buffer, count);
    for (size_t i = 0; i < count; ++i)
    {
        buffer[i] &= static_cast<const uint8_t*>(data)[i];
    }
    fseek(flash, flash_offs, SEEK_SET);
    size_t ret = fwrite(buffer, count, 1, flash);
    assert(ret == 1);
    fflush(flash);
    delete[] buffer;

    // printf("flash_write 0x%08X %u bytes\n", flash_offs, (uint32_t)count);
    // for (size_t i = 0; i < count; ++i)
    // {
    //     MemFlash[flash_offs + i] &= static_cast<const uint8_t*>(data)[i];
    // }
}

void flash_erase(uint32_t flash_offs, size_t count) 
{
    assert(flash_offs + count <= kFlashSize);
    assert(flash_offs % kFlashSectorSize == 0);
    assert(count % kFlashSectorSize == 0);

    uint8_t* buffer = new uint8_t[count];
    memset(buffer, 0xFF, count);    
    fseek(flash, flash_offs, SEEK_SET);
    size_t ret = fwrite(buffer, count, 1, flash);
    assert(ret == 1);
    fflush(flash);
    delete[] buffer;
    
    // printf("flash_erase 0x%08X %u bytes\n", flash_offs, (uint32_t)count);
    // memset(MemFlash + flash_offs, 0xFF, count);
}

// All incoming MIDI (regular PC/CC/Note *and* SysEx) is queued here by the
// libremidi callback (which runs on its own CoreMIDI thread) and drained by
// usb_midi_stream_read on the main thread, mirroring the embedded tud_midi
// stream interface. Everything is then dispatched through MidiUsb::run ->
// Controller::midiCallback, exactly like the firmware: regular messages drive
// program/scene/tuner changes, and SysEx is handled by ConfigProtocol::processSysEx
// (config) or answered as a MIDI identity request. This keeps host behavior
// identical to hardware.
static std::mutex midi_in_mutex;
static std::deque<uint8_t> midi_in_queue;

static libremidi::midi_in midi_in{
  libremidi::input_configuration{
    .on_message = [](const libremidi::message& message) {
      std::lock_guard<std::mutex> lock(midi_in_mutex);
      midi_in_queue.insert(midi_in_queue.end(), message.begin(), message.end());
    },
    .ignore_sysex = false,
  }
};

uint32_t usb_midi_available() {
  std::lock_guard<std::mutex> lock(midi_in_mutex);
  return uint32_t(midi_in_queue.size());
}

uint32_t usb_midi_stream_read(void* buffer, uint32_t bufsize) {
  std::lock_guard<std::mutex> lock(midi_in_mutex);
  uint32_t count = std::min<uint32_t>(bufsize, uint32_t(midi_in_queue.size()));
  auto* out = static_cast<uint8_t*>(buffer);
  for (uint32_t i = 0; i < count; ++i) {
    out[i] = midi_in_queue.front();
    midi_in_queue.pop_front();
  }
  return count;
}

void usb_init() {
  flash_init();
  midi.open_virtual_port("Virtual Tocata Pedal");
  midi_in.open_virtual_port("Virtual Tocata Pedal");
}

void usb_run() {
  if (!app.run()) {
    exit(0);
  }
}

size_t usb_midi_write(const unsigned char* message, size_t size) {
  midi.send_message(message, size);
  return size;
}

void board_reset() {
}

uint16_t expression_read(const HWConfigExpression& config) {
  constexpr int16_t kMargin = 1 << 5;
  constexpr int16_t kMin = kMargin;
  constexpr int16_t kMax = (1 << 12) - kMargin;
  constexpr int16_t kAbsDelta = 1 << 3;

  static int16_t exp_value = 0;
  static int16_t exp_delta = kAbsDelta;

  exp_value += exp_delta;

  if (exp_value < kMin) {
    exp_value = kMin;
    exp_delta = kAbsDelta;
  }

  if (exp_value > kMax) {
    exp_value = kMax;
    exp_delta = -kAbsDelta;
  }

  return static_cast<uint16_t>(exp_value);
}

bool is_pedal_long() {
  static bool init;
  static bool is_long;

  if (!init) {
    is_long = std::getenv("TOCATA_PEDAL_LONG");
    init = true;
  }

  return is_long;
}

}

#endif // HAL_HOST
