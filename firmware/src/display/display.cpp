#include "display.h"

#include <u8x8.h>

#include <cstdio>
#include <cassert>

namespace tocata {

/*
 * HAL callback function as prescribed by the U8G2 library.  This callback is invoked
 * to handle I2C communications.
 */
uint8_t Display::i2c_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	I2C* i2c = static_cast<I2C*>(u8x8_GetUserPtr(u8x8));
	assert(i2c);

	switch(msg) {
		case U8X8_MSG_BYTE_INIT:
			if (u8x8->bus_clock == 0)
			{
				u8x8->bus_clock = u8x8->display_info->i2c_bus_clock_100kHz * 100000UL;
			}
            i2c->init(u8x8->bus_clock);
			break;
		case U8X8_MSG_BYTE_SEND:
            i2c->sendBytes(arg_ptr, arg_int);
			break;
		case U8X8_MSG_BYTE_START_TRANSFER:
            i2c->startTransfer(u8x8_GetI2CAddress(u8x8) >> 1);
			break;
		case U8X8_MSG_BYTE_END_TRANSFER:
            i2c->endTransfer();
			break;
		default:
			break;
	}
	return 0;
} // i2c_byte_cb

/*
 * HAL callback function as prescribed by the U8G2 library.  This callback is invoked
 * to handle callbacks for GPIO and delay functions.
 */
uint8_t Display::gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	I2C* i2c = static_cast<I2C*>(u8x8_GetUserPtr(u8x8));
	assert(i2c);

	switch(msg) {
		case U8X8_MSG_DELAY_MILLI:
            i2c->delayMs(arg_int);
			break;
		default:
			break;
	}
	return 0;
} // gpio_and_delay_cb

void Display::init()
{
	setBlink(false);
	for (uint32_t i = 0; i < kNumDisplays; ++i) {
		auto u8g2 = &_u8g2[i];
		u8g2_Setup_sh1106_i2c_128x64_noname_f(u8g2, U8G2_R0, i2c_byte_cb, gpio_and_delay_cb);
		_u8g2_buffers[i].resize(u8g2_GetBufferSize(u8g2));
		u8g2_SetBufferPtr(u8g2, _u8g2_buffers[i].data());
		u8g2_SetI2CAddress(u8g2, 0x78 + (i << 1));
		u8g2_SetUserPtr(u8g2, &_i2c[i]);
		u8g2_InitDisplay(u8g2); // send init sequence to the display, display is in sleep mode after this,
		u8g2_SetPowerSave(u8g2, 0); // wake up display
		u8g2_ClearBuffer(u8g2);
		u8g2_SetFont(u8g2, u8g2_font_10x20_tf);
		u8g2_DrawStr(u8g2, 10, 30, i == 0 ? "Tocata" :  "Pedal");
		u8g2_SendBuffer(u8g2);
	}
}

void Display::setNumber(uint8_t number) {
	if (number == kNoNumber) {
		_number[0] = '\0';
	} else {
		_number[0] = (number / 10) ? '0' + (number / 10) : ' ';
		_number[1] = '0' + (number % 10);
		_number[2] = '\0';
	}
}

void Display::setText(const char* text)
{
	_scroll.text = text;
	_scroll.letter = 0;
	_scroll.pixel = 0;
	_scroll.size = text ? strlen(_scroll.text) : 0;
	_scroll.delay = 10;
}

void Display::setBlink(bool enabled)
{
	_blink.ticks = enabled ? kBlinkTicks : 0;
	_blink.enabled = enabled;
	_blink.state = true;
}

void Display::setTuner(bool enabled, uint8_t note, int8_t cents)
{
	_tuner.enabled = enabled;
	if (!enabled) {
		return;
	}

	static const char none[3] = "-";
	static const char notes[12][3] = {
		"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", 
	};

	const char* note_ptr;
	if (_tuner.isNoteValid(note)) {
		note_ptr = notes[_tuner.noteInScale(note)];
 	} else {
		note_ptr = none;
		cents = 0;
	}
	memcpy(_tuner.note, note_ptr, sizeof(_tuner.note));
	
	constexpr int8_t step = 64 / kTunerResolution;
	for (int8_t i = 0; i < kTunerResolution; ++i) {
		if (cents > -4 && cents < 4 && _tuner.isNoteValid(note)) {
			_tuner.low[i] = '-';
			_tuner.high[i] = '-';
		} else {
			int8_t low_threshold = (i - (kTunerResolution - 1)) * step;
			_tuner.low[i] = (cents < low_threshold) ? '>' : ' ';
			int8_t high_threshold = i * step;
			_tuner.high[i] = (cents > high_threshold) ? '<' : ' ';
		}	
	}
	_tuner.low[kTunerResolution] = '\0';
	_tuner.high[kTunerResolution] = '\0';
}

void Display::drawTuner()
{
	u8g2_SetFont(&_u8g2[0], u8g2_font_helvB24_tf);
	u8g2_DrawStr(&_u8g2[0], _tuner.note[1] ? 44 : 52, 17, _tuner.note);
	u8g2_SetFont(&_u8g2[0], u8g2_font_10x20_tf);
	u8g2_DrawStr(&_u8g2[0], 0, 25, _tuner.low);
	u8g2_DrawStr(&_u8g2[0], 89, 25, _tuner.high);
}

void Display::run()
{
  for (auto i = 0; i < kNumDisplays; ++i) {
	auto u8g2 = &_u8g2[i];
	u8g2_ClearBuffer(u8g2);
	u8g2_SetFontRefHeightExtendedText(u8g2);
	u8g2_SetFontPosTop(u8g2);
	u8g2_SetFontDirection(u8g2, 0);
	u8g2_SetFontMode(u8g2, 0);  
	u8g2_SetFont(u8g2, u8g2_font_7x13_mf);
    u8g2_SetDrawColor(u8g2, 1);
  }
  
  for (uint8_t i = 0; i < Program::kNumSwitches; ++i)
  {
	  drawFootswitch(i, _fs_text[i]);
  }

  if (_tuner.enabled)
  {
	drawTuner();
  }
  else 
  {
	if (_blink.enabled)
	{
		if (--_blink.ticks == 0)
		{
			_blink.ticks = kBlinkTicks;
			_blink.state = !_blink.state;
		}
	}
	if (_blink.state)
	{
		u8g2_SetFont(&_u8g2[0], u8g2_font_helvB24_tf);
		u8g2_DrawStr(&_u8g2[0], 0, 17, _number);
	}

	drawScroll();
  }

  for (auto i = 0; i < kNumDisplays; ++i) {
	auto u8g2 = &_u8g2[i];
    u8g2_SendBuffer(u8g2);
  }
}

void Display::drawScroll()
{
    constexpr uint8_t font_width = 10;
    constexpr uint8_t font_height = 20;
    constexpr uint8_t block_height = font_height;
    constexpr uint8_t start_y = 23;

	uint8_t text_offset = 0;
	for (auto i = 0; i < kNumDisplays; ++i) {
		const uint8_t max_chars = (i == 0) ? 8 : 11;
		const uint8_t block_width = font_width * max_chars;
	    const uint8_t start_x = (i == 0) ? 48 : 0;		
		char name[max_chars + 2];
		for (uint8_t i = 0; i < max_chars + 1; ++i)
		{
			name[i] = _scroll.text[_scroll.letter + text_offset + i];
		}
		name[max_chars + 1] = '\0';
		text_offset += max_chars;

		auto& u8g2 = _u8g2[i];
		u8g2_SetFont(&u8g2, u8g2_font_10x20_tf);
		u8g2_SetClipWindow(&u8g2, start_x, start_y, start_x + block_width, start_y + block_height);
		u8g2_DrawStr(&u8g2, start_x - _scroll.pixel, start_y, name);
		u8g2_SetMaxClipWindow(&u8g2);
	}
		

	if (_scroll.delay != 0)
	{
		--_scroll.delay;
		if (_scroll.delay == 0 && _scroll.letter != 0)
		{
			_scroll.delay = font_width;
			_scroll.letter = 0;
		}
		return;
	}

	if (_scroll.letter + text_offset >= _scroll.size)
	{
		_scroll.delay = font_width;
		return;
	}
	
	++_scroll.pixel;
	if (_scroll.pixel == font_width)
	{
		_scroll.pixel = 0;
		++_scroll.letter;
	}
}

void Display::drawFootswitch(uint8_t idx, const char* text)
{
  static constexpr uint8_t screen_height = 64;
  static constexpr uint8_t x_padding = 3;
  static constexpr uint8_t y_padding = 2;
  static constexpr uint8_t font_width = 7;
  static constexpr uint8_t font_height = 13;
  static constexpr uint8_t max_chars = 5;
  static constexpr uint8_t separation = 2;
  static constexpr uint8_t block_width = (2 * x_padding) + (font_width * max_chars);
  static constexpr uint8_t block_width_padded = block_width + separation;
  static constexpr uint8_t block_height = (2 * y_padding) + font_height;
  static constexpr uint8_t block_height_padded = screen_height - block_height;

  if (!text)
  {
	  return;
  }

  struct Topology {
	uint8_t display;
	uint8_t x;
	uint8_t y;
  };

  constexpr Topology topology[] {
#if TOCATA_PEDAL_LONG
	[0] = {0, 1, 0},
	[1] = {0, 2, 0},
	[2] = {1, 0, 0},
	[3] = {1, 1, 0},
	[4] = {1, 2, 0},
	[5] = {0, 1, 1},
	[6] = {0, 2, 1},
	[7] = {1, 0, 1},
	[8] = {1, 1, 1},
	[9] = {1, 2, 1},
#else
	[0] = {0, 0, 0},
	[1] = {0, 1, 0},
	[2] = {0, 2, 0},
	[3] = {0, 0, 1},
	[4] = {0, 1, 1},
	[5] = {0, 2, 1},
#endif
  };
  
  uint8_t x = block_width_padded * topology[idx].x;
  uint8_t y = block_height_padded * topology[idx].y;
  auto u8g2 = &_u8g2[topology[idx].display];
	
  if (_fs_state[idx])
  {
    u8g2_DrawFrame(u8g2, x, y, block_width, block_height);
  }
  
  u8g2_DrawStr(u8g2, x + x_padding, y + y_padding, text);
}

} // namespace tocata
