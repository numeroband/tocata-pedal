#define TEST_BUILD 0
#if TEST_BUILD

#include "filesystem.h"
#include "hal.h"

using namespace tocata;

int main() {
  puts("starting");
  auto start = millis();

  TocataFS.init(true);

  auto end = millis();
  printf("Storage init in ms: %u\n", end - start);
  TocataFS.printUsage();

  TocataFS.test();  

  return 0;
}

#else

#include "controller.h"
#include "hal.h"


static tocata::HWConfig hw_config = {
  .switches = {
    .state_machine_id = 0,
    .first_input_pin = 2,
    .first_output_pin = 6,
  },
  .leds = {
    .state_machine_id = 1,
    .data_pin = 18,
  },
  .display = {
    .sda_pin = 20,
    .scl_pin = 21,
  },
};

int main()
{
  static tocata::Controller controller{hw_config};
  controller.init();

  while (1)
  {
    controller.run();
    tocata::idle_loop();
  }
  
  return 0;
}

#endif