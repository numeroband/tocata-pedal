#include <controller.h>

static constexpr tocata::HWConfig hw_config = {
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

static tocata::Controller controller{hw_config};

int main(void)
{
  controller.init();

  while (1)
  {
    controller.run();
  }
  
  return 0;
}
