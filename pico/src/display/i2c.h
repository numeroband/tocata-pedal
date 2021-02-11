#pragma once

#include <u8g2.h>
#include "i2c.h"
#include "hal.h"

namespace tocata {

class I2C
{
public:
	I2C(const HWConfigI2C& config) : _config(config) {}
	void init(uint32_t baudrate);
	void startTransfer(uint8_t addr);
	void sendBytes(const void* buf, size_t len);
	void endTransfer();
	void delayMs(uint8_t ms);

private:
	static constexpr unsigned int kI2CTimeout = 1000;
	static constexpr unsigned int kMaxTransfer = 32;

	const HWConfigI2C& _config;
	uint8_t _addr;
	uint8_t _transfer[kMaxTransfer];
	uint32_t _transfer_len;
};

} // namespace tocata