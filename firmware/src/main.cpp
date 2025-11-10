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
      .sda_pin = 20,
      .scl_pin = 21,
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
