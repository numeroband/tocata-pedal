#include <usb_device.h>
#include <filesystem.h>
#include <switches.h>

#include "bsp/board.h"

static tocata::UsbDevice usb_device;
static tocata::Switches switches;

int main(void)
{
  board_init();

  usb_device.init();
  switches.init();
  
  uint32_t start = board_millis();
  while (1)
  {
    usb_device.run();
    switches.run();

    if (start != ~0 && (board_millis() - start) > 5000)
    {
      start = ~0;
      tocata::Storage::begin();
    }
  }

  return 0;
}
