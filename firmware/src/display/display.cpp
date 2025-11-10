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

/*
 * HAL callback function as prescribed by the U8G2 library.  This callback is invoked
 * to handle SPI communications.
 */
uint8_t Display::spi_byte_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	SPI* spi = static_cast<SPI*>(u8x8_GetUserPtr(u8x8));
	assert(spi);


	uint8_t *data;
	uint8_t internal_spi_mode;
 
	switch(msg) 
	{
		case U8X8_MSG_BYTE_SEND:
			spi->sendBytes(arg_ptr, arg_int);
      		break;
    	case U8X8_MSG_BYTE_INIT:
      		if ( u8x8->bus_clock == 0 ) 	/* issue 769 */
				u8x8->bus_clock = u8x8->display_info->sck_clock_hz;
      		/* disable chipselect */
      		u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
      
      		/* no wait required here */
			spi->init(); // u8x8->pins[U8X8_PIN_I2C_CLOCK], MISO, u8x8->pins[U8X8_PIN_I2C_DATA]);
			break;
      
    	case U8X8_MSG_BYTE_SET_DC:
      		u8x8_gpio_SetDC(u8x8, arg_int);
      		break;
      
    	case U8X8_MSG_BYTE_START_TRANSFER:
      		/* SPI mode has to be mapped to the mode of the current controller, at least Uno, Due, 101 have different SPI_MODEx values */
      		internal_spi_mode =  0;
      		switch(u8x8->display_info->spi_mode)
      		{
//				case 0: internal_spi_mode = SPI_MODE0; break;
//				case 1: internal_spi_mode = SPI_MODE1; break;
//				case 2: internal_spi_mode = SPI_MODE2; break;
//				case 3: internal_spi_mode = SPI_MODE3; break;
                default: break;
      		}
      
      		// spi->startTransactionSPI.beginTransaction(SPISettings(u8x8->bus_clock, MSBFIRST, internal_spi_mode));
      
			u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);  
			// u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
      		break;
      
    	case U8X8_MSG_BYTE_END_TRANSFER:      
      		// u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, NULL);
      		u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
			// SPI.endTransaction();
      		break;
    	default:
			break;
	}
	return 0;
}


/*
 * HAL callback function as prescribed by the U8G2 library.  This callback is invoked
 * to handle callbacks for GPIO and delay functions.
 */
uint8_t Display::spi_gpio_and_delay_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
	SPI* spi = static_cast<SPI*>(u8x8_GetUserPtr(u8x8));
	assert(spi);

	switch(msg)
	{
		case U8X8_MSG_GPIO_AND_DELAY_INIT:
			break;
  
    	case U8X8_MSG_DELAY_MILLI:
      		spi->delayMs(arg_int);
      		break;
      
	    case U8X8_MSG_GPIO_DC:
			spi->dc(arg_int);
			break;
	
    	case U8X8_MSG_GPIO_CS:
			spi->cs(arg_int);
			break;
	
    	case U8X8_MSG_GPIO_RESET:
			spi->reset(arg_int);
			break;
    	default:
            printf("!!!!! unknown gpio msg %u\n", msg);
      		return 0;
	}
	return 1;
}

void Display::init()
{
	setBlink(false);
	for (uint32_t i = 0; i < kNumDisplays; ++i) {
		auto u8g2 = &_u8g2[i];
		// u8g2_Setup_sh1106_i2c_128x64_noname_f(u8g2, U8G2_R0, i2c_byte_cb, gpio_and_delay_cb);
		u8g2_Setup_ssd1322_nhd_256x64_f(u8g2, U8G2_R0, spi_byte_cb, spi_gpio_and_delay_cb);
		_u8g2_buffers[i].resize(u8g2_GetBufferSize(u8g2));
		u8g2_SetBufferPtr(u8g2, _u8g2_buffers[i].data());
		u8g2_SetI2CAddress(u8g2, 0x78); //  + (i << 1));
		u8g2_SetUserPtr(u8g2, &_spi);
		// u8g2_SetUserPtr(u8g2, &_i2c[i]);
		u8g2_InitDisplay(u8g2); // send init sequence to the display, display is in sleep mode after this,
		u8g2_SetPowerSave(u8g2, 0); // wake up display
		u8g2_ClearBuffer(u8g2);
		u8g2_SetFont(u8g2, u8g2_font_10x20_tf);
		u8g2_DrawStr(u8g2, 10, 30, i == 0 ? "Tocata Pedal" : "");
		u8g2_SendBuffer(u8g2);
	}
}

void Display::setNumber(uint8_t number) {
	_dirty = true;
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
	uint32_t note_start = 11 * kTunerResolution;
	u8g2_SetFont(&_u8g2[0], u8g2_font_helvB24_tf);
	u8g2_DrawStr(&_u8g2[0], _tuner.note[1] ? note_start : note_start + 8, 17, _tuner.note);
	u8g2_SetFont(&_u8g2[0], u8g2_font_10x20_tf);
	u8g2_DrawStr(&_u8g2[0], 0, 25, _tuner.low);
	u8g2_DrawStr(&_u8g2[0], note_start + 45, 25, _tuner.high);
}

void Display::run()
{
	if (_dirty || _tuner.enabled) {
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

	fillBuffer(0);
  }

  for (uint8_t i = 0; i < Program::kNumSwitches; ++i)
  {
	  drawFootswitch(i, _fs_text[i], true);
  }

  for (auto i = 0; i < kNumDisplays; ++i) {
      sendBuffer(i);
  }

  _dirty = _tuner.enabled;
}

void Display::drawScroll()
{
    constexpr uint8_t font_width = 10;
    constexpr uint8_t font_height = 20;
    constexpr uint8_t block_height = font_height;
    constexpr uint8_t start_y = 23;

	uint8_t text_offset = 0;
	bool found_end = false;
	for (auto i = 0; i < kNumDisplays; ++i) {
		const uint8_t max_chars = 19; // (i == 0) ? 8 : 11;
		const uint8_t block_width = font_width * max_chars;
	    const uint8_t start_x = (i == 0) ? 48 : 0;		
		char name[max_chars + 2];
        name[0] = '\0';
		for (uint8_t i = 0; !found_end && i < max_chars + 1; ++i)
		{
			name[i] = _scroll.text[_scroll.letter + text_offset + i];
            if (name[i] == '\0')
            {
                found_end = true;
            }
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

void Display::drawFrame(uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool enabled)
{
	uint32_t start = y * (kColumns / kColsPerByte) + (x / kColsPerByte);
	memset(&_spi_buffer[start], enabled ? 0xFF : 0, width / kColsPerByte);
	start = (y + height - 1) * (kColumns / kColsPerByte) + (x / kColsPerByte);
	memset(&_spi_buffer[start], enabled ? 0xFF : 0, width / kColsPerByte);
	for (uint32_t side_y = y + 1; side_y < (y + height - 1); ++side_y) {
		start = side_y * (kColumns / kColsPerByte) + (x / kColsPerByte);
		_spi_buffer[start] = enabled ? 0xF0 : 0;
		start += (width - 2) / 2;
		_spi_buffer[start] = enabled ? 0x0F : 0;
	}
}

void Display::drawFootswitch(uint8_t idx, const char* text, bool draw_frame)
{
  static constexpr uint8_t screen_height = 64;
  static constexpr uint8_t x_padding = 3;
  static constexpr uint8_t y_padding = 2;
  static constexpr uint8_t font_width = 7;
  static constexpr uint8_t font_height = 13;
  static constexpr uint8_t separation = 2;

  const uint8_t max_chars = is_pedal_long() ? 8 : 5;
  const uint8_t block_width = (2 * x_padding) + (font_width * max_chars);
  const uint8_t block_width_padded = block_width + separation;
  const uint8_t block_height = (2 * y_padding) + font_height;
  const uint8_t block_height_padded = screen_height - block_height;

  uint8_t x = block_width_padded * _topology[idx].x;
  uint8_t y = block_height_padded * _topology[idx].y;
  if (draw_frame)
  {
	drawFrame(x, y, block_width, block_height, text && _fs_state[idx]);
	return;
  }

	if (!text) {
		return;
	}

	auto u8g2 = &_u8g2[_topology[idx].display];

	char trunc_text[max_chars + 1];
	strncpy(trunc_text, text, max_chars);
	trunc_text[max_chars] = '\0';
	u8g2_DrawStr(u8g2, x + x_padding, y + y_padding, trunc_text);
}

void Display::startTransfer(u8x8_t* u8x8) {
    spi_byte_cb(u8x8, U8X8_MSG_BYTE_START_TRANSFER, 0, nullptr);
}

void Display::endTransfer(u8x8_t* u8x8) {
    spi_byte_cb(u8x8, U8X8_MSG_BYTE_END_TRANSFER, 0, nullptr);
}

void Display::sendCommand(u8x8_t* u8x8, uint8_t command) {
    spi_byte_cb(u8x8, U8X8_MSG_BYTE_SET_DC, 0, nullptr);
    spi_byte_cb(u8x8, U8X8_MSG_BYTE_SEND, 1, &command);
}

void Display::sendData(u8x8_t* u8x8, std::span<uint8_t> data) {
    spi_byte_cb(u8x8, U8X8_MSG_BYTE_SET_DC, 1, nullptr);
    _spi.sendBytes(data.data(), data.size());
}

void Display::fillBuffer(size_t idx) {
    const uint8_t* tiles = _u8g2_buffers[idx].data();
    constexpr size_t cols = kColumns / kColsPerByte;
    constexpr size_t rows = kRows;
    constexpr size_t rowsPerByte = 8;
    size_t row = 0;
    size_t col = 0;
    for (size_t i = 0; i < kColumns * kRows / rowsPerByte; i = i + 4) {
        const uint8_t* col_tiles = tiles + i;
        for (size_t bit = 0; bit < rowsPerByte; ++bit) {
            size_t bit_row = row + bit;
            uint8_t* ram = &_spi_buffer[bit_row * cols + col];
            ram[0] = 0;
            ram[1] = 0;
            if ((col_tiles[0] >> bit) & 1) { ram[0] |= 0xF0; }
            if ((col_tiles[1] >> bit) & 1) { ram[0] |= 0x0F; }
            if ((col_tiles[2] >> bit) & 1) { ram[1] |= 0xF0; }
            if ((col_tiles[3] >> bit) & 1) { ram[1] |= 0x0F; }
        }
        col += kColsPerByte;
        if (col >= cols) {
            row += rowsPerByte;
            col = 0;
        }
    }
}

void Display::sendBuffer(size_t idx)
{
    auto u8g2 = &_u8g2[idx];
	// u8g2_SendBuffer(u8g2);

    auto u8x8 = u8g2_GetU8x8(u8g2);
    startTransfer(u8x8);
    std::array<uint8_t, 2> rowRange{0, kRamRows - 1};
    std::array<uint8_t, 2> colRange{kColsOffset, kRamColumns - 1 + kColsOffset};
    sendCommand(u8x8, kSetRowAddressCommand);
    sendData(u8x8, rowRange);
    sendCommand(u8x8, kSetColumnAddressCommand);
    sendData(u8x8, colRange);
    sendCommand(u8x8, kWriteRamCommand);
    sendData(u8x8, _spi_buffer);
    endTransfer(u8x8);
}

} // namespace tocata
