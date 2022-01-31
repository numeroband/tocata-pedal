#pragma once

#include <config.h>
#include <u8g2.h>
#include "i2c.h"

#include <bitset>
#include <array>

namespace tocata {

class Display
{
public:
	static constexpr uint8_t kNoNumber = 0xFF;
	Display(const HWConfigI2C& config, const std::bitset<Program::kNumSwitches>& fs_state) : _i2c(config), _fs_state(fs_state) {}
	void init();
	void run();
	void setNumber(uint8_t number);
	void setText(const char* text);
	void setFootswitch(uint8_t idx, const char* text) { _fs_text[idx] = text; }
	void clearSwitches() { _fs_text = {}; }
	void setBlink(bool enabled);
	
private:
	static constexpr uint8_t kBlinkTicks = 8;

	static uint8_t i2c_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
	static uint8_t gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
	
	void drawFootswitch(uint8_t idx, const char* text);
	void drawScroll();

	u8g2_t _u8g2;
	I2C _i2c;
	bool _dirty = true;
	char _number[3] = "";
	const char* _text = "";
	std::array<const char*, Program::kNumSwitches> _fs_text{};
	const std::bitset<Program::kNumSwitches>& _fs_state{};

	struct {
		const char* text;
		uint8_t letter;
		uint8_t pixel;
		uint8_t size;
		uint8_t delay;
	} _scroll;

	struct {
		uint8_t ticks;
		bool enabled;
		bool state;
	} _blink;
};

} // namespace tocata