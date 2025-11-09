#include "spi.h"

#include <cstdio>
#include <cassert>
#include <cstring>

namespace tocata {

void SPI::init()
{
	spi_init();
}
	
void SPI::sendBytes(const void* buf, size_t len)
{
	spi_transfer(static_cast<const uint8_t*>(buf), len);
}

void SPI::delayMs(uint8_t ms)
{
	sleep_ms(ms);
}

} // namespace tocata
