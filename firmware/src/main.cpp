#include "controller.h"
#include "hal.h"

int main()
{
  static tocata::HWConfig hw_config = {
    .switches = {
      .state_machine_id = 0,
      .first_input_pin = 2,
      .first_output_pin = 6,
    },
    .leds = {
      .state_machine_id = 1,
#ifdef CYW43_WL_GPIO_LED_PIN    
      .data_pin = 28,
#else
      .data_pin = 18,
#endif
    },
    .display = {
      {
        .index = 0,
        .sda_pin = 20,
        .scl_pin = 21,
      },
      {
        .index = 1,
#ifdef CYW43_WL_GPIO_LED_PIN    
        .sda_pin = 18,
        .scl_pin = 19,
#else
        .sda_pin = 22,
        .scl_pin = 23,
#endif
      },
    },
    .expression = {
      .adc_pin = 26,
    }
  };

  static tocata::Controller controller{hw_config};
  controller.init();

  while (true)
  {
    controller.run();
    tocata::idle_loop();
  }
  
  return 0;
}
