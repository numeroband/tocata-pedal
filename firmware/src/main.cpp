#include "controller.h"
#include "hal.h"

int main()
{
  static tocata::HWConfig hw_config = {
#ifdef RASPBERRYPI_PICO2
    .switches = {
      .state_machine_id = 0,
      .first_input_pin = 0,
      .map = { 8, 9, 6, 7, 3, 2, 1, 0, 4, 5, },
    },
    .leds = {
      .state_machine_id = 1,
      .data_pin = 28,
      .map = { 3, 2, 1, 0, 4, 5, 6, 7, },
    },
    .displaySPI = {
      .available = true,
      .clk_pin = 10,
      .tx_pin = 11,
      .dc_pin = 14,
      .reset_pin = 15,
    },
    .expression = {
      .adc_pin = 27,
      .connected_pin = 26,
    },
    .ethernet = {
      .available = true,
      .rx_pin = 16,
      .cs_pin = 17,
      .clk_pin = 18,
      .tx_pin = 19,
      .reset_pin = 20,
      .irq_pin = 21,
    },
#elif defined(RASPBERRYPI_PICO)
    .switches = {
      .state_machine_id = 0,
      .first_input_pin = 2,
      .first_output_pin = 6,
    },
    .leds = {
      .state_machine_id = 1,
      .data_pin = 18,
    },
    .displayI2C = {
      .available = true,
      .sda_pin = 20,
      .scl_pin = 21,
    },
    .expression = {
      .adc_pin = 26,
    },
#else
    .leds = {
      .map = { 0, 1, 2, 3, 4, 5, 6, 7, },
    },
    .switches = {
      .map = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, },
    },
#endif
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
