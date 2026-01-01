# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")
cmake_minimum_required(VERSION 3.12)

if (NOT "${PICO_BOARD}" STREQUAL "none")
# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)
endif()

project(tocata_pedal C CXX ASM)
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)

set(TOCATA_PEDAL_PATH ${PROJECT_SOURCE_DIR})
set(TOCATA_PEDAL_VERSION_MAJOR 0)
set(TOCATA_PEDAL_VERSION_MINOR 4)
set(TOCATA_PEDAL_VERSION_SUBMINOR 1)

if (NOT "${PICO_BOARD}" STREQUAL "none")
# Initialize the SDK
pico_sdk_init()
endif()


set(BOARD_NAME W6100_EVB_PICO2)
set(WIZNET_CHIP W6100)
add_definitions(-D_WIZCHIP_=W6100)
add_definitions(-DDEVICE_BOARD_NAME=W6100_EVB_PICO2)

# Add libs
add_subdirectory(lib)

# Add source code
add_subdirectory(src)
