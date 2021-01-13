#pragma once

#include "config.h"

#include <U8g2lib.h>

namespace tocata {

class Display
{
public:
	Display();
	void begin();
	void loop();
	void setConnected(bool connected) { _connected = connected; }
	void setProgram(Program& program);
	
private:
	void drawFootswitch(const Program::Footswitch& footswitch);
	void drawScroll();

	U8G2_SH1106_128X64_NONAME_F_HW_I2C _u8g2;
	bool _connected = false;
	bool _dirty = true;
	Program* _program = nullptr;
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

}