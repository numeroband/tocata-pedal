#include "usb/device.h"

#include "bsp/board.h"

static tocata::UsbDevice usb_device;

int main(void)
{
  board_init();

  usb_device.init();

  while (1)
  {
    usb_device.run();
  }

  return 0;
}
