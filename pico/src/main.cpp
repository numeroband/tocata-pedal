#include <usb_device.h>
#include <filesystem.h>

#include "bsp/board.h"

static tocata::UsbDevice usb_device;

int main(void)
{
  board_init();

  tocata::TocataFS.begin();
  usb_device.init();

  while (1)
  {
    usb_device.run();
  }

  return 0;
}
