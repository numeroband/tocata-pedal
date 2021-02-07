#include "usb_device.h"
#include "usb_descriptors.h"
#include "bsp/board.h"

extern "C" {
#include <pico/stdio.h>
#include <pico/stdio/driver.h>
}

static struct stdio_driver usb_stdio = {
  .out_chars = [](const char *buf, int len) {
    tud_cdc_write(buf, len);
  },
  .out_flush = []() {
    tud_cdc_write_flush();
  },
  .in_chars = [](char *buf, int len) { 
    return (int)tud_cdc_read(buf, (uint32_t)len); 
  },
  .crlf_enabled = PICO_STDIO_DEFAULT_CRLF,
};


//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  tocata::UsbDevice::singleton().mount();
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  tocata::UsbDevice::singleton().umount();
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  tocata::UsbDevice::singleton().suspend();
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  tocata::UsbDevice::singleton().resume();
}

namespace tocata
{

UsbDevice* UsbDevice::_singleton;

void UsbDevice::init()
{
  assert(!_singleton);

  _singleton = this;

  _midi.init();
  _web.init();

  tusb_init();
  stdio_set_driver_enabled(&usb_stdio, true);
}

void UsbDevice::run()
{
  tud_task(); // tinyusb device task
  blink();
  _midi.run();
  _web.run();
}

void UsbDevice::blink()
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // Blink every interval ms
  if ( board_millis() - start_ms < _blink_interval_ms) return; // not enough time
  start_ms += _blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}

}