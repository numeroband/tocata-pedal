#include "i2c.h"

#include <hardware/i2c.h>
#include <hardware/gpio.h>

#include <cstdio>
#include <cassert>
#include <cstring>

namespace tocata {

void I2C::init(uint32_t baudrate)
{
	i2c_init(i2c0, baudrate);
	gpio_set_function(_pin_sda, GPIO_FUNC_I2C);
	gpio_set_function(_pin_scl, GPIO_FUNC_I2C);
	gpio_pull_up(_pin_sda);
	gpio_pull_up(_pin_scl);

	printf("Init I2C with %u sda: %u scl: %u\n", baudrate, _pin_sda, _pin_scl);
}
	
void I2C::startTransfer(uint8_t addr)
{
	_addr = addr;
	_transfer_len = 0;
}

void I2C::sendBytes(const void* buf, size_t len)
{
	assert(_transfer_len + len <= kMaxTransfer);
	memcpy(_transfer + _transfer_len, buf, len);
	_transfer_len += len;
}

void I2C::endTransfer()
{
	i2c_write_blocking(i2c0, _addr, _transfer, _transfer_len, false);

	// printf("I2C %u bytes to %02X: ",_transfer_len, _addr);
	// for (uint8_t i = 0; i < _transfer_len; ++i)
	// {
	// 	printf("%02X ", _transfer[i]);
	// }
	// printf("\n");
}

void I2C::delayMs(uint8_t ms)
{
	sleep_ms(ms);
}

} // namespace tocata
