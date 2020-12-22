#include "display.h"

namespace tocata {

Display::Display() : _u8g2(U8G2_R0) {}

void Display::begin()
{
  _u8g2.begin();
}

void Display::setProgram(uint8_t program)
{
	if (program != _program)
	{
		_dirty = true;
	}
	_program = program;
}

void Display::loop()
{
    _u8g2.clearBuffer();
	_u8g2.setFontRefHeightExtendedText();
	_u8g2.setFontPosTop();
	_u8g2.setFontDirection(0);
	_u8g2.setFontMode(0);  
	_u8g2.setFont(u8g2_font_7x13_mf);
	int idx = 0;
	drawSwitch(idx++, "REVRB", true);
	drawSwitch(idx++, " EQ  ", true);
	drawSwitch(idx++, "DISTR", true);
	drawSwitch(idx++, "DELAY", true);
	drawSwitch(idx++, "CHORS", true);
	drawSwitch(idx++, "FLANG", true);

	_u8g2.setDrawColor(1);
	_u8g2.setFont(u8g2_font_10x20_tf);
	_u8g2.drawStr( 35, 23, "FELL ON B");  

	_u8g2.setFont(u8g2_font_helvB18_tf);
	_u8g2.drawStr( 0, 19, "99");  
    _u8g2.sendBuffer();
}

void Display::drawSwitch(int idx, const char* text, bool enabled)
{
  int screen_height = 64;
  int x_padding = 3;
  int y_padding = 2;
  int font_width = 7;
  int font_height = 13;
  int max_chars = 5;
  int separation = 2;
  int block_width = (2 * x_padding) + (font_width * max_chars);
  int block_width_padded = block_width + separation;
  int block_height = (2 * y_padding) + font_height;
  int block_height_padded = screen_height - block_height;
  
  int x = block_width_padded * (idx % 3);
  int y = block_height_padded * (idx / 3);

  _u8g2.setDrawColor(1);
  
  if (enabled)
  {
    _u8g2.drawBox(x, y, block_width, block_height);
    _u8g2.setDrawColor(0);    
  }
  
  _u8g2.drawStr(x + x_padding, y + y_padding, text);
}

}