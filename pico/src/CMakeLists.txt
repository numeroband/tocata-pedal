# set(CMAKE_C_FLAGS_DEBUG "-O0 -g")

add_executable(tocata_pedal)

if(PICO_SDK)
pico_generate_pio_header(tocata_pedal ${CMAKE_CURRENT_LIST_DIR}/pio/switch_matrix.pio)
pico_generate_pio_header(tocata_pedal ${CMAKE_CURRENT_LIST_DIR}/pio/ws2812.pio)
endif()

# target_compile_definitions(tocata_pedal PRIVATE U8X8_WITH_USER_PTR)

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

if(PICO_SDK)
target_link_libraries(tocata_pedal PRIVATE 
        pico_stdlib 
        hardware_i2c
        hardware_flash
        hardware_pio
        tinyusb_device 
        tinyusb_board
        u8g2
        )
else()
target_link_libraries(tocata_pedal PRIVATE u8g2)
endif()

if(PICO_SDK)
# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(tocata_pedal)
endif()