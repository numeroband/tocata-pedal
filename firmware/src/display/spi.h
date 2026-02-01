#pragma once

#include <u8g2.h>
#include "hal.h"

namespace tocata {

class SPI
{
public:
	SPI(const HWConfigDisplaySPI& config) : _config(config) {}

	void init() {
		spi_init(_config);
	}

	void sendBytes(const void* buf, size_t len) {
		spi_transfer(static_cast<const uint8_t*>(buf), len);
	}

	void delayMs(uint8_t ms) {
		sleep_ms(ms);
	}

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
	const HWConfigDisplaySPI& _config;
};

} // namespace tocata