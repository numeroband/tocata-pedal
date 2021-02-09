#include <usb_device.h>
#include <filesystem.h>
#include <switches.h>
#include <display.h>

static tocata::UsbDevice usb_device;
static tocata::Switches switches;
static tocata::Display display{8, 9};

int main(void)
{
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, 1);

  usb_device.init();
  switches.init();
  display.init();
  tocata::Program program{0};
  display.setProgram(0, program);
  
  uint32_t start = to_ms_since_boot(get_absolute_time());
  while (1)
  {
    usb_device.run();
    switches.run();
    display.run();

    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (start != ~0 && (now - start) > 5000)
    {
      start = ~0;
      tocata::Storage::begin();
    }
  }

  return 0;
}
