#include "display.h"

namespace tocata {

Display::Display() : _u8g2(U8G2_R0) {}

void Display::begin()
{
  	_u8g2.begin();
	_u8g2.setFont(u8g2_font_10x20_tf);
	_u8g2.drawStr(10, 30, "Tocata Pedal");
    _u8g2.sendBuffer();
}

void Display::setProgram(Program& program)
{
	_program = &program;
	utoa(program.id() + 1, _program_str, 10);
	_scroll.name = _program->available() ? _program->name() : "<EMPTY>";
	_scroll.letter = 0;
	_scroll.pixel = 0;
	_scroll.size = strlen(_scroll.name);
	_scroll.delay = 10;
}

void Display::loop()
{
    _u8g2.clearBuffer();
	_u8g2.setFontRefHeightExtendedText();
	_u8g2.setFontPosTop();
	_u8g2.setFontDirection(0);
	_u8g2.setFontMode(0);  
	_u8g2.setFont(u8g2_font_7x13_mf);
	if (_program->available())
	{
		uint8_t num_switches = _program->numFootswitches();
		for (uint8_t i = 0; i < num_switches; ++i)
		{
			drawFootswitch(_program->footswitch(i));
		}
	}

	_u8g2.setDrawColor(1);

	_u8g2.setFont(u8g2_font_helvB18_tf);
	_u8g2.drawStr( 0, 19, _program_str);

	drawScroll();

    _u8g2.sendBuffer();
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

	_u8g2.setFont(u8g2_font_10x20_tf);
	_u8g2.setClipWindow(start_x, start_y, start_x + block_width, start_y + block_height);
	_u8g2.drawStr(start_x - _scroll.pixel, start_y, name);
	_u8g2.setMaxClipWindow();

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

void Display::drawFootswitch(const Program::Footswitch& footswitch)
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

  uint8_t idx = footswitch.id();
  uint8_t x = block_width_padded * (idx % 3);
  uint8_t y = block_height_padded * (idx / 3);

  _u8g2.setDrawColor(1);
  
  if (footswitch.enabled())
  {
    _u8g2.drawFrame(x, y, block_width, block_height);
  }
  
  _u8g2.drawStr(x + x_padding, y + y_padding, footswitch.name());
}

}