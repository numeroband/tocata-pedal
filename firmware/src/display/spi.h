#pragma once

#include <u8g2.h>
#include "i2c.h"
#include "hal.h"

namespace tocata {

class SPI
{
public:
	SPI(const HWConfigSPI& config) : _config(config) {}
	void init();
	void sendBytes(const void* buf, size_t len);
	void delayMs(uint8_t ms);
	void cs(bool enabled) {
		spi_set_cs(enabled);
	}

	void dc(bool enabled) {
		spi_set_dc(enabled);
	}

	void reset(bool enabled) {
		spi_set_reset(enabled);
	}

private:
	const HWConfigSPI& _config;
};

} // namespace tocata