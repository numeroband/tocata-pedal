# set(CMAKE_C_FLAGS_DEBUG "-O0 -g")

add_executable(tocata_pedal
        main.cpp
        usb/usb_descriptors.c
        usb/device.cpp
        usb/web_usb.cpp
        usb/midi_usb.cpp
        )

target_include_directories(tocata_pedal PRIVATE ${CMAKE_CURRENT_LIST_DIR} usb)

target_link_libraries(tocata_pedal PRIVATE 
        pico_stdlib 
        tinyusb_device 
        tinyusb_board
        )

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(tocata_pedal)
