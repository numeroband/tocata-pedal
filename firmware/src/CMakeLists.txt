# set(CMAKE_C_FLAGS_DEBUG "-O0 -g")

add_executable(tocata_pedal)

target_sources(tocata_pedal PRIVATE
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
        display/i2c.cpp
        display/display.cpp
        )

target_include_directories(tocata_pedal PRIVATE 
        ${CMAKE_CURRENT_LIST_DIR} 
        usb 
        config 
        pio
        display
        hal
        ../lib/websocketpp
        ../lib/asio/asio/include
        )

target_link_libraries(tocata_pedal PRIVATE u8g2)

target_compile_definitions(tocata_pedal PRIVATE
        VERSION_MAJOR=${TOCATA_PEDAL_VERSION_MAJOR}
        VERSION_MINOR=${TOCATA_PEDAL_VERSION_MINOR}
        VERSION_SUBMINOR=${TOCATA_PEDAL_VERSION_SUBMINOR}
        )

if(PICO_SDK)
pico_generate_pio_header(tocata_pedal ${CMAKE_CURRENT_LIST_DIR}/pio/switch_matrix.pio)
pico_generate_pio_header(tocata_pedal ${CMAKE_CURRENT_LIST_DIR}/pio/ws2812b.pio)

pico_set_program_name(tocata_pedal "tocata-pedal")
pico_set_program_version(tocata_pedal "${TOCATA_PEDAL_VERSION_MAJOR}.${TOCATA_PEDAL_VERSION_MINOR}.${TOCATA_PEDAL_VERSION_SUBMINOR}")

pico_set_binary_type(tocata_pedal copy_to_ram)

pico_enable_stdio_uart(tocata_pedal 0)
pico_enable_stdio_usb(tocata_pedal 0)

target_link_libraries(tocata_pedal PRIVATE 
        pico_stdlib 
        pico_unique_id
        hardware_i2c
        hardware_flash
        hardware_pio
        tinyusb_device 
        tinyusb_board
        )
 
# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(tocata_pedal)
endif()
