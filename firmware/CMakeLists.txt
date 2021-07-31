cmake_minimum_required(VERSION 3.12)

# Pull in PICO SDK (must be before project)
if(CMAKE_C_COMPILER MATCHES "^.*arm-none-eabi-gcc.*$")
include(pico_sdk_import.cmake)
endif()

project(tocata_pedal C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(TOCATA_PEDAL_PATH ${PROJECT_SOURCE_DIR})
set(TOCATA_PEDAL_VERSION_MAJOR 0)
set(TOCATA_PEDAL_VERSION_MINOR 2)
set(TOCATA_PEDAL_VERSION_SUBMINOR 0)

if(CMAKE_C_COMPILER MATCHES "^.*arm-none-eabi-gcc.*$")
# Initialize the SDK
pico_sdk_init()
endif()

# Add libs
add_subdirectory(lib)

# Add source code
add_subdirectory(src)
