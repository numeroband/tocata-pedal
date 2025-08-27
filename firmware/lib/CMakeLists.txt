add_library(u8g2 INTERFACE)

set(U8G2_PATH ${PROJECT_SOURCE_DIR}/lib/u8g2)

target_sources(u8g2 INTERFACE
    ${U8G2_PATH}/csrc/u8x8_setup.c
    ${U8G2_PATH}/csrc/u8x8_u8toa.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1306_96x16.c
    ${U8G2_PATH}/csrc/u8g2_d_memory.c
    ${U8G2_PATH}/csrc/u8g2_selection_list.c
    ${U8G2_PATH}/csrc/u8g2_hvline.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1306_128x64_noname.c
    ${U8G2_PATH}/csrc/u8x8_message.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1306_48x64.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1305.c
    ${U8G2_PATH}/csrc/u8g2_kerning.c
    ${U8G2_PATH}/csrc/u8x8_input_value.c
    ${U8G2_PATH}/csrc/u8x8_d_st7586s_s028hn118a.c
    ${U8G2_PATH}/csrc/u8g2_polygon.c
    ${U8G2_PATH}/csrc/u8x8_d_uc1604.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1306_128x32.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1322.c
    ${U8G2_PATH}/csrc/u8x8_d_sbn1661.c
    ${U8G2_PATH}/csrc/u8g2_input_value.c
    ${U8G2_PATH}/csrc/u8x8_8x8.c
    ${U8G2_PATH}/csrc/u8x8_debounce.c
    ${U8G2_PATH}/csrc/u8x8_d_uc1638.c
    ${U8G2_PATH}/csrc/u8g2_setup.c
    ${U8G2_PATH}/csrc/u8x8_d_ist3020.c
    ${U8G2_PATH}/csrc/u8x8_display.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1317.c
    ${U8G2_PATH}/csrc/u8x8_d_ks0108.c
    ${U8G2_PATH}/csrc/u8g2_box.c
    ${U8G2_PATH}/csrc/u8x8_selection_list.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1326.c
    ${U8G2_PATH}/csrc/u8x8_d_st75256.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1309.c
    ${U8G2_PATH}/csrc/u8x8_byte.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1607_200x200.c
    ${U8G2_PATH}/csrc/u8x8_d_pcd8544_84x48.c
    ${U8G2_PATH}/csrc/u8log.c
    ${U8G2_PATH}/csrc/u8x8_d_sh1107.c
    ${U8G2_PATH}/csrc/u8log_u8x8.c
    ${U8G2_PATH}/csrc/u8g2_message.c
    ${U8G2_PATH}/csrc/u8x8_d_uc1610.c
    ${U8G2_PATH}/csrc/u8x8_d_sh1106_72x40.c
    ${U8G2_PATH}/csrc/u8g2_line.c
    ${U8G2_PATH}/csrc/u8g2_intersection.c
    ${U8G2_PATH}/csrc/u8g2_fonts.c
    ${U8G2_PATH}/csrc/u8x8_d_sh1106_64x32.c
    ${U8G2_PATH}/csrc/u8x8_d_sed1330.c
    ${U8G2_PATH}/csrc/u8x8_d_pcf8814_hx1230.c
    ${U8G2_PATH}/csrc/u8x8_d_il3820_296x128.c
    ${U8G2_PATH}/csrc/u8x8_d_uc1701_dogs102.c
    ${U8G2_PATH}/csrc/u8g2_cleardisplay.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1325.c
    ${U8G2_PATH}/csrc/u8g2_font.c
    ${U8G2_PATH}/csrc/u8x8_d_uc1601.c
    ${U8G2_PATH}/csrc/u8x8_d_pcf8812.c
    ${U8G2_PATH}/csrc/u8x8_cad.c
    ${U8G2_PATH}/csrc/u8x8_d_uc1611.c
    ${U8G2_PATH}/csrc/u8x8_string.c
    ${U8G2_PATH}/csrc/u8g2_ll_hvline.c
    ${U8G2_PATH}/csrc/u8x8_fonts.c
    ${U8G2_PATH}/csrc/u8x8_d_ld7032_60x32.c
    ${U8G2_PATH}/csrc/u8x8_d_max7219.c
    ${U8G2_PATH}/csrc/u8x8_d_st7588.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1606_172x72.c
    ${U8G2_PATH}/csrc/u8x8_d_st7586s_erc240160.c
    ${U8G2_PATH}/csrc/u8x8_d_stdio.c
    ${U8G2_PATH}/csrc/u8x8_d_sh1122.c
    ${U8G2_PATH}/csrc/u8log_u8g2.c
    ${U8G2_PATH}/csrc/u8g2_circle.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1306_64x48.c
    ${U8G2_PATH}/csrc/u8g2_buffer.c
    ${U8G2_PATH}/csrc/u8g2_bitmap.c
    ${U8G2_PATH}/csrc/u8x8_d_lc7981.c
    ${U8G2_PATH}/csrc/u8x8_d_st7920.c
    ${U8G2_PATH}/csrc/u8x8_d_t6963.c
    ${U8G2_PATH}/csrc/u8x8_d_st7567.c
    ${U8G2_PATH}/csrc/u8x8_d_ls013b7dh03.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1327.c
    ${U8G2_PATH}/csrc/u8x8_d_sh1108.c
    ${U8G2_PATH}/csrc/u8x8_gpio.c
    ${U8G2_PATH}/csrc/u8x8_d_uc1608.c
    ${U8G2_PATH}/csrc/u8x8_d_uc1617.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1329.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1306_64x32.c
    ${U8G2_PATH}/csrc/u8g2_d_setup.c
    ${U8G2_PATH}/csrc/u8x8_d_uc1701_mini12864.c
    ${U8G2_PATH}/csrc/u8x8_u16toa.c
    ${U8G2_PATH}/csrc/u8x8_d_a2printer.c
    ${U8G2_PATH}/csrc/u8x8_d_st7565.c

    ${U8G2_PATH}/csrc/u8x8_capture.c
    ${U8G2_PATH}/csrc/u8x8_d_ist7920.c
    ${U8G2_PATH}/csrc/u8x8_d_s1d15e06.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1306_2040x16.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1306_72x40.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1306_128x32.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1316.c
    ${U8G2_PATH}/csrc/u8x8_d_ssd1318.c
    ${U8G2_PATH}/csrc/u8x8_d_st7511.c
    ${U8G2_PATH}/csrc/u8x8_d_st7528.c
    ${U8G2_PATH}/csrc/u8x8_d_st75320.c
    ${U8G2_PATH}/csrc/u8x8_d_st7571.c
    ${U8G2_PATH}/csrc/u8x8_d_st7586s_ymc240160.c

    # ${U8G2_PATH}/csrc/u8x8_d_ch1120.c
    # ${U8G2_PATH}/csrc/u8x8_d_gp1247ai.c
    # ${U8G2_PATH}/csrc/u8x8_d_gp1287ai.c
    # ${U8G2_PATH}/csrc/u8x8_d_gp1294ai.c
    # ${U8G2_PATH}/csrc/u8x8_d_gu800.c
    # ${U8G2_PATH}/csrc/u8x8_d_hd44102.c
    # ${U8G2_PATH}/csrc/u8x8_d_ist3020.c
    # ${U8G2_PATH}/csrc/u8x8_d_ist3088.c
    # ${U8G2_PATH}/csrc/u8x8_d_s1d15300.c
    # ${U8G2_PATH}/csrc/u8x8_d_s1d15721.c
    # ${U8G2_PATH}/csrc/u8x8_d_ssd1306_96x40.c
    # ${U8G2_PATH}/csrc/u8x8_d_ssd1312.c
    # ${U8G2_PATH}/csrc/u8x8_d_ssd1320.c
    # ${U8G2_PATH}/csrc/u8x8_d_ssd1362.c
    # ${U8G2_PATH}/csrc/u8x8_d_ssd1363.c
    # ${U8G2_PATH}/csrc/u8x8_d_st7302.c
    # ${U8G2_PATH}/csrc/u8x8_d_st75160.c
    # ${U8G2_PATH}/csrc/u8x8_d_st75161.c
    # ${U8G2_PATH}/csrc/u8x8_d_st7539.c
    # ${U8G2_PATH}/csrc/u8x8_d_st7586s_jlx320160.c
    # ${U8G2_PATH}/csrc/u8x8_d_st7586s_jlx384160.c
    # ${U8G2_PATH}/csrc/u8x8_d_st7586s_md240128.c
    # ${U8G2_PATH}/csrc/u8x8_d_uc1609.c
    # ${U8G2_PATH}/csrc/u8x8_d_uc1628.c
    )

target_compile_definitions(u8g2 INTERFACE 
    U8X8_WITH_USER_PTR
    U8G2_USE_DYNAMIC_ALLOC
    )

target_include_directories(u8g2 INTERFACE 
    ${U8G2_PATH}/csrc
    )

if(PICO_SDK)
# Ethernet
set(WIZNET_DIR ${PROJECT_SOURCE_DIR}/lib/ioLibrary_Driver)

add_library(ETHERNET_FILES STATIC)

target_sources(ETHERNET_FILES PUBLIC
        ${WIZNET_DIR}/Ethernet/socket.c
        ${WIZNET_DIR}/Ethernet/wizchip_conf.c
        )

target_include_directories(ETHERNET_FILES INTERFACE
        ${WIZNET_DIR}/Ethernet
        ${WIZNET_DIR}/Ethernet/W5500
        )

target_link_libraries(ETHERNET_FILES PUBLIC
        W5500_FILES
        DHCP_FILES
        )

add_library(W5500_FILES STATIC)

target_sources(W5500_FILES PUBLIC
        ${WIZNET_DIR}/Ethernet/W5500/w5500.c
        )

target_include_directories(W5500_FILES INTERFACE
        ${WIZNET_DIR}/Ethernet
        ${WIZNET_DIR}/Ethernet/W5500
        )

target_link_libraries(W5500_FILES PRIVATE
        ETHERNET_FILES
        )

# DHCP
add_library(AAC_FILES STATIC)

target_sources(AAC_FILES PUBLIC
        ${WIZNET_DIR}/Internet/AAC/AddressAutoConfig.c
        )

target_include_directories(AAC_FILES PUBLIC
        ${WIZNET_DIR}/Ethernet
        ${WIZNET_DIR}/Internet/AAC
        )


# DHCP
add_library(DHCP_FILES STATIC)

target_sources(DHCP_FILES PUBLIC
        ${WIZNET_DIR}/Internet/DHCP/dhcp.c
        )

target_include_directories(DHCP_FILES PUBLIC
        ${WIZNET_DIR}/Ethernet
        ${WIZNET_DIR}/Internet/DHCP
        )

# DNS
add_library(DNS_FILES STATIC)

target_sources(DNS_FILES PUBLIC
        ${WIZNET_DIR}/Internet/DNS/dns.c
        )

target_include_directories(DNS_FILES PUBLIC
        ${WIZNET_DIR}/Ethernet
        ${WIZNET_DIR}/Internet/DNS
        )
        
# HTTP Server
add_library(HTTPSERVER_FILES STATIC)

target_sources(HTTPSERVER_FILES PUBLIC
        ${WIZNET_DIR}/Internet/httpServer/httpParser.c
        ${WIZNET_DIR}/Internet/httpServer/httpServer.c
        ${WIZNET_DIR}/Internet/httpServer/httpUtil.c
        )

target_include_directories(HTTPSERVER_FILES PUBLIC
        ${WIZNET_DIR}/Ethernet
        ${WIZNET_DIR}/Internet/httpServer
        )

# SNTP
add_library(SNTP_FILES STATIC)

target_sources(SNTP_FILES PUBLIC
        ${WIZNET_DIR}/Internet/SNTP/sntp.c
        )

target_include_directories(SNTP_FILES PUBLIC
        ${WIZNET_DIR}/Ethernet
        ${WIZNET_DIR}/Internet/SNTP
        )

# Loopback
add_library(LOOPBACK_FILES STATIC)

target_sources(LOOPBACK_FILES PUBLIC
        ${WIZNET_DIR}/Application/loopback/loopback.c
        )

target_include_directories(LOOPBACK_FILES PUBLIC
        ${WIZNET_DIR}/Ethernet
        ${WIZNET_DIR}/Application/loopback
        )

# MIDI
set(ARDUINO_MIDI_PATH ${PROJECT_SOURCE_DIR}/lib/arduino_midi_library/src)
add_library(arduino_midi INTERFACE
    ${ARDUINO_MIDI_PATH}/midi_Namespace.h
    ${ARDUINO_MIDI_PATH}/midi_Defs.h
    ${ARDUINO_MIDI_PATH}/midi_Message.h
    ${ARDUINO_MIDI_PATH}/midi_Platform.h
    ${ARDUINO_MIDI_PATH}/midi_Settings.h
    ${ARDUINO_MIDI_PATH}/MIDI.cpp
    ${ARDUINO_MIDI_PATH}/MIDI.hpp
    ${ARDUINO_MIDI_PATH}/MIDI.h
    ${ARDUINO_MIDI_PATH}/serialMIDI.h
)        
target_include_directories(arduino_midi INTERFACE ${ARDUINO_MIDI_PATH})

set(ARDUINO_ETHERNET_PATH ${PROJECT_SOURCE_DIR}/lib/arduino_ethernet/src)
add_library(arduino_ethernet INTERFACE)
target_include_directories(arduino_ethernet INTERFACE ${ARDUINO_ETHERNET_PATH})

set(ARDUINO_APPLEMIDI_PATH ${PROJECT_SOURCE_DIR}/lib/Arduino-AppleMIDI-Library/src)
add_library(arduino_applemidi INTERFACE
    ${ARDUINO_APPLEMIDI_PATH}/utility/Deque.h
    ${ARDUINO_APPLEMIDI_PATH}/utility/endian.h
    ${ARDUINO_APPLEMIDI_PATH}/AppleMIDI_Debug.h
    ${ARDUINO_APPLEMIDI_PATH}/AppleMIDI_Defs.h
    ${ARDUINO_APPLEMIDI_PATH}/AppleMIDI_Namespace.h
    ${ARDUINO_APPLEMIDI_PATH}/AppleMIDI_Parser.h
    ${ARDUINO_APPLEMIDI_PATH}/AppleMIDI_Participant.h
    ${ARDUINO_APPLEMIDI_PATH}/AppleMIDI_PlatformBegin.h
    ${ARDUINO_APPLEMIDI_PATH}/AppleMIDI_PlatformEnd.h
    ${ARDUINO_APPLEMIDI_PATH}/AppleMIDI_Settings.h
    ${ARDUINO_APPLEMIDI_PATH}/AppleMIDI.cpp
    ${ARDUINO_APPLEMIDI_PATH}/AppleMIDI.h
    ${ARDUINO_APPLEMIDI_PATH}/AppleMIDI.hpp
    ${ARDUINO_APPLEMIDI_PATH}/rtp_Defs.h
    ${ARDUINO_APPLEMIDI_PATH}/rtpMIDI_Clock.h
    ${ARDUINO_APPLEMIDI_PATH}/rtpMIDI_Defs.h
    ${ARDUINO_APPLEMIDI_PATH}/rtpMIDI_Parser_CommandSection.hpp
    ${ARDUINO_APPLEMIDI_PATH}/rtpMIDI_Parser_JournalSection.hpp
    ${ARDUINO_APPLEMIDI_PATH}/rtpMIDI_Parser.h
)
target_include_directories(arduino_applemidi INTERFACE ${ARDUINO_APPLEMIDI_PATH})
target_link_libraries(arduino_applemidi INTERFACE 
    arduino_ethernet
    arduino_midi
    )


else()
add_subdirectory(libremidi)
add_subdirectory(SDL)
endif()
