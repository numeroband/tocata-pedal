#pragma once

#include <config.h>
#include <u8g2.h>
#include "i2c.h"

#include <bitset>

namespace tocata {

class Display
{
public:
	Display(const HWConfigI2C& config, const std::bitset<Program::kNumSwitches>& fs_state) : _i2c(config), _fs_state(fs_state) {}
	void init();
	void run();
	void setConnected(bool connected) { _connected = connected; }
	void setProgram(uint8_t id, const Program& program);
	
private:
	static uint8_t i2c_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
	static uint8_t gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

	void drawFootswitch(uint8_t idx, const Program::Footswitch& footswitch);
	void drawScroll();

	u8g2_t _u8g2;
	I2C _i2c;
	bool _connected = false;
	bool _dirty = true;
	const Program* _program = nullptr;
	const std::bitset<Program::kNumSwitches>& _fs_state;
	char _program_str[3];
	struct {
		const char* name;
		uint8_t letter;
		uint8_t pixel;
		uint8_t size;
		uint8_t delay;
	} _scroll;
	const char* _program_name;
};

} // namespace tocata