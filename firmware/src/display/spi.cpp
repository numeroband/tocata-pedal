#include "spi.h"

#include <cstdio>
#include <cassert>
#include <cstring>

namespace tocata {

void SPI::init(uint32_t baudrate)
{
	i2c_init(baudrate, _config);

	printf("Init I2C with %u baudrate\n", baudrate);
}
	
void SPI::sendBytes(const void* buf, size_t len)
{
	spi_transfer(static_cast<const uint8_t*>(buf), len);
}

void SPI::delayMs(uint8_t ms)
{
	sleep_ms(ms);
}

void SPI::cs(bool enabled) {
}

void SPI::dc(bool enabled) {
    set_dc(enabled);
}

void SPI::reset(bool enabled) {
}

} // namespace tocata
