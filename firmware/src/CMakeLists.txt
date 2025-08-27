# set(CMAKE_C_FLAGS_DEBUG "-O0 -g")

set(CUR_TARGET TocataPedal)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
add_executable(${CUR_TARGET} MACOSX_BUNDLE)
set_target_properties(${CUR_TARGET} PROPERTIES
    BUNDLE True
    MACOSX_BUNDLE_GUI_IDENTIFIER my.domain.style.identifier.${CUR_TARGET}
    MACOSX_BUNDLE_BUNDLE_NAME ${CUR_TARGET}
)
else()
add_executable(${CUR_TARGET})
endif()

target_sources(${CUR_TARGET} PRIVATE
        main.cpp
        controller.cpp
        hal/hal_host.cpp
        hal/hal_pico.cpp
        usb/usb_device.cpp
        usb/web_usb.cpp
        usb/midi_usb.cpp
        config/config.cpp
        config/flash_partition.cpp
        config/filesystem.cpp
        pio/switches.cpp
        pio/leds.cpp
        pio/expression.cpp
        display/i2c.cpp
        display/display.cpp
        )

target_include_directories(${CUR_TARGET} PRIVATE 
        ${CMAKE_CURRENT_LIST_DIR} 
        usb 
        config 
        pio
        display
        ethernet
        hal
        ../lib/websocketpp
        ../lib/asio/asio/include
        )

target_link_libraries(${CUR_TARGET} PRIVATE u8g2)

target_compile_definitions(${CUR_TARGET} PRIVATE
        VERSION_MAJOR=${TOCATA_PEDAL_VERSION_MAJOR}
        VERSION_MINOR=${TOCATA_PEDAL_VERSION_MINOR}
        VERSION_SUBMINOR=${TOCATA_PEDAL_VERSION_SUBMINOR}
        )

if(PICO_SDK)
pico_generate_pio_header(${CUR_TARGET} ${CMAKE_CURRENT_LIST_DIR}/pio/switches.pio)
pico_generate_pio_header(${CUR_TARGET} ${CMAKE_CURRENT_LIST_DIR}/pio/ws2812b.pio)

pico_set_program_name(${CUR_TARGET} "tocata-pedal")
pico_set_program_version(${CUR_TARGET} "${TOCATA_PEDAL_VERSION_MAJOR}.${TOCATA_PEDAL_VERSION_MINOR}.${TOCATA_PEDAL_VERSION_SUBMINOR}")

pico_set_binary_type(${CUR_TARGET} copy_to_ram)

pico_enable_stdio_uart(${CUR_TARGET} 0)
pico_enable_stdio_usb(${CUR_TARGET} 0)

target_link_libraries(${CUR_TARGET} PRIVATE 
        pico_stdlib 
        pico_unique_id
        pico_cyw43_arch_none
        hardware_i2c
        hardware_flash
        hardware_pio
        hardware_adc
        hardware_spi
        tinyusb_device 
        tinyusb_board
        )

# Ethernet
set(WIZCHIP_SPI_SCLK_SPEED 43)
add_definitions(-D_WIZCHIP_SPI_SCLK_SPEED=${WIZCHIP_SPI_SCLK_SPEED})
set(WIZNET_CHIP W5500)
add_definitions(-D_WIZCHIP_=5500)

target_sources(${CUR_TARGET} PRIVATE
        ethernet/wizchip_spi.c
        ethernet/timer.c
        ethernet/eth.cpp
        )

target_link_libraries(${CUR_TARGET} PRIVATE 
        ETHERNET_FILES 
        )

target_link_libraries(${CUR_TARGET} PRIVATE
        arduino_midi
        arduino_applemidi        
        )
# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(${CUR_TARGET})
else()
target_link_libraries(${CUR_TARGET} PRIVATE 
        libremidi
        SDL2-static
        )
endif()
