#include <controller.h>
#include <hal.h>

static constexpr tocata::HWConfig hw_config = {
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

static tocata::Controller controller{hw_config};

int main(void)
{
  controller.init();

  while (1)
  {
    controller.run();
    tocata::idle_loop();
  }
  
  return 0;
}
