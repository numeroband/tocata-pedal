#pragma once

#include <config.h>
#include <u8g2.h>
#include "i2c.h"

#include <bitset>
#include <array>
#include <vector>

namespace tocata {

class Display
{
public:
	static constexpr uint8_t kNoNumber = 0xFF;
	Display(const HWConfigI2C& config, const std::bitset<Program::kNumSwitches>& fs_state) :
		_i2c{
			config,
#if TOCATA_PEDAL_LONG
			config,
#endif
		}, _fs_state(fs_state) {}
	void init();
	void run();
	void setNumber(uint8_t number);
	void setText(const char* text);
	void setFootswitch(uint8_t idx, const char* text) { _fs_text[idx] = text; }
	void clearSwitches() { _fs_text = {}; }
	void setBlink(bool enabled);
	void setTuner(bool enabled, uint8_t note = 0, int8_t cents = 0);
	
private:
	static constexpr uint8_t kBlinkTicks = 8;
	static constexpr uint8_t kNumDisplays = TOCATA_PEDAL_LONG ? 2 : 1;

	static uint8_t i2c_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
	static uint8_t gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
	
	void drawFootswitch(uint8_t idx, const char* text);
	void drawScroll();
	void drawTuner();

	std::array<u8g2_t, kNumDisplays> _u8g2;
	std::array<I2C, kNumDisplays> _i2c;
	bool _dirty = true;
	char _number[3] = "";
	const char* _text = "";
	std::array<const char*, Program::kNumSwitches> _fs_text{};
	const std::bitset<Program::kNumSwitches>& _fs_state{};
	std::array<std::vector<uint8_t>, kNumDisplays> _u8g2_buffers;

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
	
	static constexpr int8_t kTunerResolution = 4;
	struct {
		char note[3];
		char low[kTunerResolution + 1];
		char high[kTunerResolution + 1];
		bool enabled;
		static bool isNoteValid(uint8_t note) {
			return note >= 24;
		}
		static uint8_t noteInScale(uint8_t note) {
			return (note - 24) % 12;
		}
	} _tuner{};
};

} // namespace tocata