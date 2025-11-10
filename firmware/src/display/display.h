#pragma once

#include <config.h>
#include <u8g2.h>
#include "i2c.h"
#include "spi.h"

#include <bitset>
#include <array>
#include <vector>
#include <span>

namespace tocata {

class Display
{
public:
	static constexpr uint8_t kNoNumber = 0xFF;
	Display(const HWConfigI2C* config, const std::bitset<Program::kNumSwitches>& fs_state) :
		_i2c{
			config[0],
			config[1],
        },
        _spi{config[0]}, _fs_state{fs_state} 
	{
		if (is_pedal_long()) {
			// _topology[0] = {0, 0, 0};
			// _topology[1] = {0, 1, 0};
			// _topology[2] = {1, 0, 0};
			// _topology[3] = {1, 1, 0};
			// _topology[4] = {0, 0, 1};
			// _topology[5] = {0, 1, 1};
			// _topology[6] = {1, 0, 1};
			// _topology[7] = {1, 1, 1};
			_topology[0] = {0, 0, 0};
			_topology[1] = {0, 1, 0};
			_topology[2] = {0, 2, 0};
			_topology[3] = {0, 3, 0};
			_topology[4] = {0, 0, 1};
			_topology[5] = {0, 1, 1};
			_topology[6] = {0, 2, 1};
			_topology[7] = {0, 3, 1};
} else {
			_topology[0] = {0, 0, 0};
			_topology[1] = {0, 1, 0};
			_topology[2] = {0, 2, 0};
			_topology[3] = {0, 0, 1};
			_topology[4] = {0, 1, 1};
			_topology[5] = {0, 2, 1};
		}
	}
	void init();
	void run();
	void setNumber(uint8_t number);
	void setText(const char* text);
	void setFootswitch(uint8_t idx, const char* text) { _fs_text[idx] = text; _dirty = true; }
	void clearSwitches() { _fs_text = {}; }
	void setBlink(bool enabled);
	void setTuner(bool enabled, uint8_t note = 0, int8_t cents = 0);
	
private:
	static constexpr uint8_t kBlinkTicks = 8;
    static constexpr size_t kColumns = 256;
    static constexpr size_t kRows = 64;
    static constexpr size_t kRamRows = kRows;
    static constexpr size_t kRamColumns = kColumns / 4;
    static constexpr size_t kColsPerByte = 2;
    static constexpr size_t kColsOffset = 28;
    static constexpr uint8_t kSetRowAddressCommand = 0x75;
    static constexpr uint8_t kSetColumnAddressCommand = 0x15;
    static constexpr uint8_t kWriteRamCommand = 0x5C;
    const uint8_t kNumDisplays = 1;
    
	static uint8_t i2c_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
	static uint8_t gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

	static uint8_t spi_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
	static uint8_t spi_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

	void drawFootswitch(uint8_t idx, const char* text, bool draw_frame = false);
	void drawFrame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool enabled);
	void drawScroll();
	void drawTuner();
    void sendBuffer(size_t idx);
    void fillBuffer(size_t idx);
    void startTransfer(u8x8_t* u8x8);
    void endTransfer(u8x8_t* u8x8);
    void sendCommand(u8x8_t* u8x8, uint8_t command);
    void sendData(u8x8_t* u8x8, std::span<uint8_t> data);

	std::array<u8g2_t, kMaxDisplays> _u8g2{};
	std::array<I2C, kMaxDisplays> _i2c;
	SPI _spi;
	bool _dirty = true;
	char _number[3] = "";
	const char* _text = "";
	std::array<const char*, Program::kNumSwitches> _fs_text{};
	const std::bitset<Program::kNumSwitches>& _fs_state{};
	std::array<std::vector<uint8_t>, kMaxDisplays> _u8g2_buffers{};
    std::array<uint8_t, (kColumns / kColsPerByte) * kRows> _spi_buffer;

	struct {
		const char* text;
		uint8_t letter;
		uint8_t pixel;
		uint8_t size;
		uint8_t delay;
	} _scroll{};

	struct {
		uint8_t ticks;
		bool enabled;
		bool state;
	} _blink{};
	
	static constexpr int8_t kTunerResolution = 10;
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

  struct Topology {
	uint8_t display;
	uint8_t x;
	uint8_t y;
  };
  
  Topology _topology[Program::kNumSwitches]{};
};

} // namespace tocata
