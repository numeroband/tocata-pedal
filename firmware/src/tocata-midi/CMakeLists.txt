# Dual mode. On macOS this is add_subdirectory()'d by ../CMakeLists.txt as part
# of the host-sim project. On Linux it is configured directly:
#
#     cmake -S firmware/src/tocata-midi -B firmware/build-linux
#
# so a Linux box needs neither the Pico SDK nor the u8g2 / SDL / libremidi
# submodules -- only asio and websocketpp.
#
# PROJECT_IS_TOP_LEVEL is no use here: it is set by project(), which is exactly
# what we are deciding whether to call. Compare the source dirs instead.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # Kept inside this branch so it does not reset policy scope when this file
    # is pulled in as a subdirectory of the (3.12) firmware project.
    cmake_minimum_required(VERSION 3.16)
    project(tocata_midi CXX)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    # Read the version out of the firmware's top-level CMakeLists rather than
    # duplicating it here, where it would silently drift.
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../../CMakeLists.txt TOCATA_TOP_CMAKE)
    foreach(PART MAJOR MINOR SUBMINOR)
        string(REGEX MATCH "set\\(TOCATA_PEDAL_VERSION_${PART}[ \t]+([0-9]+)\\)"
               TOCATA_VERSION_MATCH "${TOCATA_TOP_CMAKE}")
        # Test the match, not CMAKE_MATCH_1: a legitimate version component of
        # "0" is falsy to if().
        if(TOCATA_VERSION_MATCH STREQUAL "")
            message(FATAL_ERROR
                "Could not parse TOCATA_PEDAL_VERSION_${PART} from ../../CMakeLists.txt")
        endif()
        set(TOCATA_PEDAL_VERSION_${PART} ${CMAKE_MATCH_1})
    endforeach()
    unset(TOCATA_TOP_CMAKE)
    unset(TOCATA_VERSION_MATCH)
    message(STATUS "TocataMidi standalone build, version "
        "${TOCATA_PEDAL_VERSION_MAJOR}.${TOCATA_PEDAL_VERSION_MINOR}.${TOCATA_PEDAL_VERSION_SUBMINOR}")
endif()

set(CUR_TARGET TocataMidi)

if(APPLE)
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
        )

target_include_directories(${CUR_TARGET} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/../../lib/asio/asio/include
        ${CMAKE_CURRENT_LIST_DIR}/../../lib/websocketpp
        )

target_compile_definitions(${CUR_TARGET} PRIVATE
        VERSION_MAJOR=${TOCATA_PEDAL_VERSION_MAJOR}
        VERSION_MINOR=${TOCATA_PEDAL_VERSION_MINOR}
        VERSION_SUBMINOR=${TOCATA_PEDAL_VERSION_SUBMINOR}
        )

# The MIDI backend is picked by midi_port.hpp; this only has to supply the
# matching library.
if(APPLE)
target_link_libraries(${CUR_TARGET} PRIVATE
        "-framework CoreMIDI"
        "-framework CoreFoundation"
        )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
# FindALSA ships with CMake and has defined ALSA::ALSA since 3.12, so no
# pkg-config dependency. asio still instantiates std::thread/std::mutex
# internals under ASIO_STANDALONE, hence Threads.
find_package(ALSA REQUIRED)
find_package(Threads REQUIRED)
target_link_libraries(${CUR_TARGET} PRIVATE ALSA::ALSA Threads::Threads)
# websocketpp 0.8.2 predates C++20's ban on template-ids in constructor names.
target_compile_options(${CUR_TARGET} PRIVATE -Wno-template-id-cdtor)
else()
message(FATAL_ERROR "TocataMidi: no MIDI backend for ${CMAKE_SYSTEM_NAME}")
endif()
