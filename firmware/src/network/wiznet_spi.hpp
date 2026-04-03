#pragma once

#include "hal_pico.h"

#include <array>
#include <cstdint>

namespace tocata {

#define SPI_PORT spi0

class EthernetSPI
{
public:
    void init() {
        critical_section_init(&_critical_section);

        uint baudrate = spi_init(SPI_PORT, SPI_CLK * 1000 * 1000);
        printf("SPI baudrate: %u\n", baudrate);

        gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
        gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
        gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

        // make the SPI pins available to picotool
        bi_decl(bi_3pins_with_func(PIN_MISO, PIN_MOSI, PIN_SCK, GPIO_FUNC_SPI));

        // chip select is active-low, so we'll initialise it to a driven-high state
        gpio_init(PIN_CS);
        gpio_set_dir(PIN_CS, GPIO_OUT);
        gpio_put(PIN_CS, 1);

        // make the SPI pins available to picotool
        bi_decl(bi_1pin_with_name(PIN_CS, "W6100 CHIP SELECT"));

        _dma_tx = dma_claim_unused_channel(true);
        _dma_rx = dma_claim_unused_channel(true);

        _dma_channel_config_tx = dma_channel_get_default_config(_dma_tx);
        channel_config_set_transfer_data_size(&_dma_channel_config_tx, DMA_SIZE_8);
        channel_config_set_dreq(&_dma_channel_config_tx, DREQ_SPI0_TX);

        // We set the inbound DMA to transfer from the SPI receive FIFO to a memory buffer paced by the SPI RX FIFO DREQ
        // We coinfigure the read address to remain unchanged for each element, but the write
        // address to increment (so data is written throughout the buffer)
        _dma_channel_config_rx = dma_channel_get_default_config(_dma_rx);
        channel_config_set_transfer_data_size(&_dma_channel_config_rx, DMA_SIZE_8);
        channel_config_set_dreq(&_dma_channel_config_rx, DREQ_SPI0_RX);
        channel_config_set_read_increment(&_dma_channel_config_rx, false);
        channel_config_set_write_increment(&_dma_channel_config_rx, true);

        reset();
    }

    template<typename T>
    void write(uint16_t address, uint8_t block, T value) {
        uint8_t buffer[sizeof(value) + 3]{
            uint8_t(address >> 8),
            uint8_t(address),
            uint8_t(block | SPI_WRITE),
        };

        for (size_t i = 0; i < sizeof(value); ++i) {
            buffer[3 + sizeof(value) - i - 1] = uint8_t(value >> (8 * i));
        }

        select();
        spi_write_blocking(SPI_PORT, reinterpret_cast<const uint8_t*>(buffer), sizeof(buffer));
        deselect();
    }
    
    template<typename T>
    T read(uint16_t address, uint8_t block) {
        T value = 0;
        uint8_t buffer[sizeof(value) + 3]{
            uint8_t(address >> 8),
            uint8_t(address),
            uint8_t(block | SPI_READ),
        };

        select();
        spi_write_read_blocking(SPI_PORT, 
            reinterpret_cast<const uint8_t*>(buffer), 
            reinterpret_cast<uint8_t*>(buffer), 
            sizeof(buffer));
        deselect();

        for (size_t i = 0; i < sizeof(value); ++i) {
            value |= buffer[3 + sizeof(value) - i - 1] << (8 * i);
        }

        return value;
    }

    void write(uint16_t address, uint8_t block, const void* buffer, uint16_t size) {
        uint8_t header[]{
            uint8_t(address >> 8),
            uint8_t(address),
            uint8_t(block | SPI_WRITE),
        };

        select();
        spi_write_blocking(SPI_PORT, reinterpret_cast<const uint8_t*>(header), sizeof(header));
        if (size < kMinDMATransfer) {
            spi_write_blocking(SPI_PORT, reinterpret_cast<const uint8_t*>(buffer), size);
        } else {
            writeBurst(buffer, size);
        }
        deselect();
    }

    void read(uint16_t address, uint8_t block, void* buffer, uint16_t size) {
        uint8_t header[]{
            uint8_t(address >> 8),
            uint8_t(address),
            uint8_t(block | SPI_READ),
        };

        select();
        spi_write_blocking(SPI_PORT, reinterpret_cast<const uint8_t*>(header), sizeof(header));
        if (size < kMinDMATransfer) {
            spi_read_blocking(SPI_PORT, 0, reinterpret_cast<uint8_t*>(buffer), size);
        } else {
            readBurst(buffer, size);
        }
        deselect();
    }

private:
    static constexpr uint16_t kMinDMATransfer = 64;

    static constexpr uint SPI_CLK = 50;
    static constexpr uint PIN_SCK = 18;
    static constexpr uint PIN_MOSI = 19;
    static constexpr uint PIN_MISO = 16;
    static constexpr uint PIN_CS = 17;
    static constexpr uint PIN_RST = 20;
    static constexpr uint PIN_INT = 21;

    static constexpr uint8_t SPI_READ = 0 << 2;
    static constexpr uint8_t SPI_WRITE = 1 << 2;

    void reset() {
        gpio_init(PIN_RST);
        gpio_set_dir(PIN_RST, GPIO_OUT);

        gpio_put(PIN_RST, 0);
        sleep_us(1);
        gpio_put(PIN_RST, 1);
        sleep_ms(100);

        // make the SPI pins available to picotool
        bi_decl(bi_1pin_with_name(PIN_RST, "W6100 RESET"));
    }

    void select() {
        critical_section_enter_blocking(&_critical_section);
        gpio_put(PIN_CS, 0);
    }

    void deselect() {
        gpio_put(PIN_CS, 1);
        critical_section_exit(&_critical_section);
    }

    void readBurst(void* buffer, uint16_t len) {
        uint8_t dummy_data = 0xFF;

        channel_config_set_read_increment(&_dma_channel_config_tx, false);
        channel_config_set_write_increment(&_dma_channel_config_tx, false);
        dma_channel_configure(_dma_tx, &_dma_channel_config_tx,
                            &spi_get_hw(SPI_PORT)->dr, // write address
                            &dummy_data,               // read address
                            len,                       // element count (each element is of size transfer_data_size)
                            false);                    // don't start yet

        channel_config_set_read_increment(&_dma_channel_config_rx, false);
        channel_config_set_write_increment(&_dma_channel_config_rx, true);
        dma_channel_configure(_dma_rx, &_dma_channel_config_rx,
                            buffer,                    // write address
                            &spi_get_hw(SPI_PORT)->dr, // read address
                            len,                       // element count (each element is of size transfer_data_size)
                            false);                    // don't start yet

        dma_start_channel_mask((1u << _dma_tx) | (1u << _dma_rx));
        dma_channel_wait_for_finish_blocking(_dma_rx);
    }

    void writeBurst(const void* buffer, uint16_t len) {
        uint8_t dummy_data;

        channel_config_set_read_increment(&_dma_channel_config_tx, true);
        channel_config_set_write_increment(&_dma_channel_config_tx, false);
        dma_channel_configure(_dma_tx, &_dma_channel_config_tx,
                            &spi_get_hw(SPI_PORT)->dr, // write address
                            buffer,                    // read address
                            len,                       // element count (each element is of size transfer_data_size)
                            false);                    // don't start yet

        channel_config_set_read_increment(&_dma_channel_config_rx, false);
        channel_config_set_write_increment(&_dma_channel_config_rx, false);
        dma_channel_configure(_dma_rx, &_dma_channel_config_rx,
                            &dummy_data,               // write address
                            &spi_get_hw(SPI_PORT)->dr, // read address
                            len,                       // element count (each element is of size transfer_data_size)
                            false);                    // don't start yet

        dma_start_channel_mask((1u << _dma_tx) | (1u << _dma_rx));
        dma_channel_wait_for_finish_blocking(_dma_rx);
    }

    uint _dma_tx;
    uint _dma_rx;
    dma_channel_config _dma_channel_config_tx;
    dma_channel_config _dma_channel_config_rx;
    critical_section_t _critical_section;
};

}