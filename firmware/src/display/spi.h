#pragma once

#include <u8g2.h>
#include "i2c.h"
#include "hal.h"

namespace tocata {

class SPI
{
public:
	SPI(const HWConfigI2C& config) : _config(config) {}
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
	static constexpr unsigned int kI2CTimeout = 1000;
	static constexpr unsigned int kMaxTransfer = 32;

	const HWConfigI2C& _config;
	uint8_t _addr;
	uint8_t _transfer[kMaxTransfer];
	uint32_t _transfer_len;
};

} // namespace tocata