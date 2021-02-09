#pragma once

#include <u8g2.h>
#include "i2c.h"

namespace tocata {

class I2C
{
public:
	struct HWConfig
	{
		uint8_t sda_pin;
		uint8_t scl_pin;		
	};

	I2C(const HWConfig& config) : _pin_sda(config.sda_pin), _pin_scl(config.scl_pin) {}
	I2C() {}
	void init(uint32_t baudrate);
	void startTransfer(uint8_t addr);
	void sendBytes(const void* buf, size_t len);
	void endTransfer();
	void delayMs(uint8_t ms);

private:
	static constexpr unsigned int kI2CTimeout = 1000;
	static constexpr unsigned int kMaxTransfer = 32;

	uint8_t _pin_sda;
	uint8_t _pin_scl;
	// i2c_inst_t* _i2c;
	uint8_t _addr;
	uint8_t _transfer[kMaxTransfer];
	uint32_t _transfer_len;
};

} // namespace tocata