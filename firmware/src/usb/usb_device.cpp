#include "usb_device.h"
#include "hal.h"

#include <cassert>

#define DELAY_BOOT_MS 0

namespace tocata
{

UsbDevice* UsbDevice::_singleton;

void UsbDevice::init()
{
  assert(!_singleton);

  _singleton = this;

  board_led_init();

  _midi.init();
  _web.init();

  usb_init();

  uint32_t start = millis();
  while (millis() - start < DELAY_BOOT_MS)
  {
    blink();
    usb_run();
  }
}

void UsbDevice::run()
{
  usb_run();
  blink();
  _midi.run();
  _web.run();
}

void UsbDevice::blink()
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // Blink every interval ms
  if (millis() - start_ms < _blink_interval_ms) return; // not enough time
  start_ms += _blink_interval_ms;

  board_led_enable(led_state);
  led_state = 1 - led_state; // toggle
}

}