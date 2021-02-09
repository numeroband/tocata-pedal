#pragma once

#include <u8g2.h>
#include "i2c.h"

namespace tocata {

class I2C
{
public:
	I2C(uint8_t pin_sda, uint8_t pin_scl) : _pin_sda(pin_sda), _pin_scl(pin_scl) {}
	void init(uint32_t baudrate);
	void startTransfer(uint8_t addr);
	void sendBytes(const void* buf, size_t len);
	void endTransfer();

	void setClock(uint8_t value);
	void setData(uint8_t value);
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