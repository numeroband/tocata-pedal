#pragma once

#include "midi_usb.h"
#include "web_usb.h"

#include <tusb.h>

namespace tocata
{

class UsbDevice
{
public:
  static UsbDevice& singleton() { return *_singleton; }
  void mount() { _blink_interval_ms = BLINK_MOUNTED; }
  void umount() { _blink_interval_ms = BLINK_NOT_MOUNTED; }
  void suspend() { _blink_interval_ms = BLINK_SUSPENDED; }
  void resume() { _blink_interval_ms = BLINK_MOUNTED; }

  UsbDevice(WebUsb::Delegate& web_delegate) : _web(web_delegate) {}
  void init();
  void run();

  MidiUsb& midi() { return _midi; };
  WebUsb& web() { return _web; }

private:
  enum  {
    BLINK_NOT_MOUNTED = 250,
    BLINK_MOUNTED = 1000,
    BLINK_SUSPENDED = 2500,

    BLINK_ALWAYS_ON   = UINT32_MAX,
    BLINK_ALWAYS_OFF  = 0
  };

  void blink();

  static UsbDevice* _singleton;

  MidiUsb _midi;
  WebUsb _web;
  uint32_t _blink_interval_ms = BLINK_NOT_MOUNTED;
};

}