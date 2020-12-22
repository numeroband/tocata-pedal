#pragma once

#include <U8g2lib.h>

namespace tocata {

class Display
{
public:
	Display();
	void begin();
	void loop();
	void setConnected(bool connected) { _connected = connected; }
	void setProgram(uint8_t program);
	
private:
	void drawSwitch(int idx, const char* text, bool enabled);

	U8G2_SH1106_128X64_NONAME_F_HW_I2C _u8g2;
	bool _connected = false;
	int _program = 0;
	bool _dirty = true;
};

}