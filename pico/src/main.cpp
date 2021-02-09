#include <controller.h>

static tocata::Controller controller;

int main(void)
{
  static constexpr tocata::Controller::HWConfig hw_config = {
    .switches = {
      .state_machine_id = 0,
      .first_input_pin = 2,
      .first_output_pin = 6,
    },
    .display = {
      .sda_pin = 8,
      .scl_pin = 9,
    },
  };

  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, 1);

  controller.init(hw_config);

  while (1)
  {
    controller.run();
  }
  
  return 0;
}
