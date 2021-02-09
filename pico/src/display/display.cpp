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
            i2c->startTransfer(u8x8_GetI2CAddress(u8x8));
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
		case U8X8_MSG_GPIO_I2C_CLOCK:
            i2c->setClock(arg_int);
			break;
		case U8X8_MSG_GPIO_I2C_DATA:
            i2c->setClock(arg_int);
			break;
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
	u8g2_Setup_sh1106_i2c_128x64_noname_f(&_u8g2, U8G2_R0, i2c_byte_cb, gpio_and_delay_cb);
	u8g2_SetUserPtr(&_u8g2, &_i2c);
	u8g2_InitDisplay(&_u8g2); // send init sequence to the display, display is in sleep mode after this,
	u8g2_SetPowerSave(&_u8g2, 0); // wake up display
    u8g2_ClearBuffer(&_u8g2);
	u8g2_SetFont(&_u8g2, u8g2_font_10x20_tf);
	u8g2_DrawStr(&_u8g2, 10, 30, "Tocata Pedal");
    u8g2_SendBuffer(&_u8g2);
}

void Display::setProgram(uint8_t id, const Program& program)
{
	_program = &program;
	sprintf(_program_str, "%2u", id);
	_scroll.name = _program->available() ? _program->name() : "<EMPTY>";
	_scroll.letter = 0;
	_scroll.pixel = 0;
	_scroll.size = strlen(_scroll.name);
	_scroll.delay = 10;
}

void Display::run()
{
    u8g2_ClearBuffer(&_u8g2);
	u8g2_SetFontRefHeightExtendedText(&_u8g2);
	u8g2_SetFontPosTop(&_u8g2);
	u8g2_SetFontDirection(&_u8g2, 0);
	u8g2_SetFontMode(&_u8g2, 0);  
	u8g2_SetFont(&_u8g2, u8g2_font_7x13_mf);
	if (_program->available())
	{
		uint8_t num_switches = _program->numFootswitches();
		for (uint8_t i = 0; i < num_switches; ++i)
		{
			drawFootswitch(i, _program->footswitch(i));
		}
	}

	u8g2_SetDrawColor(&_u8g2, 1);

	u8g2_SetFont(&_u8g2, u8g2_font_helvB18_tf);
	u8g2_DrawStr(&_u8g2,  0, 19, _program_str);

	drawScroll();

    u8g2_SendBuffer(&_u8g2);
}

void Display::drawScroll()
{
    static constexpr uint8_t font_width = 10;
    static constexpr uint8_t font_height = 20;
    static constexpr uint8_t max_chars = 9;
    static constexpr uint8_t block_width = font_width * max_chars;
    static constexpr uint8_t block_height = font_height;
    static constexpr uint8_t start_x = 33;
    static constexpr uint8_t start_y = 23;

	char name[max_chars + 2];
	for (uint8_t i = 0; i < max_chars + 1; ++i)
	{
		name[i] = _scroll.name[_scroll.letter + i];
	}
	name[max_chars + 1] = '\0';

	u8g2_SetFont(&_u8g2, u8g2_font_10x20_tf);
	u8g2_SetClipWindow(&_u8g2, start_x, start_y, start_x + block_width, start_y + block_height);
	u8g2_DrawStr(&_u8g2, start_x - _scroll.pixel, start_y, name);
	u8g2_SetMaxClipWindow(&_u8g2);

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

	if (_scroll.letter + max_chars >= _scroll.size)
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

void Display::drawFootswitch(uint8_t idx, const Program::Footswitch& footswitch)
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

  if (!footswitch.available())
  {
	  return;
  }

  uint8_t x = block_width_padded * (idx % 3);
  uint8_t y = block_height_padded * (idx / 3);

  u8g2_SetDrawColor(&_u8g2, 1);
  
  if (footswitch.enabled())
  {
    u8g2_DrawFrame(&_u8g2, x, y, block_width, block_height);
  }
  
  u8g2_DrawStr(&_u8g2, x + x_padding, y + y_padding, footswitch.name());
}

} // namespace tocata
