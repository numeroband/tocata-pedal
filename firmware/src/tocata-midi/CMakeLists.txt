set(CUR_TARGET TocataMidi)
add_executable(${CUR_TARGET} MACOSX_BUNDLE)
set_target_properties(${CUR_TARGET} PROPERTIES
    BUNDLE True
    MACOSX_BUNDLE_GUI_IDENTIFIER my.domain.style.identifier.${CUR_TARGET}
    MACOSX_BUNDLE_BUNDLE_NAME ${CUR_TARGET}
)

target_sources(${CUR_TARGET} PRIVATE
        main.cpp
        )

target_include_directories(${CUR_TARGET} PRIVATE 
        ../../lib/asio/asio/include
        )

target_compile_definitions(${CUR_TARGET} PRIVATE
        VERSION_MAJOR=${TOCATA_PEDAL_VERSION_MAJOR}
        VERSION_MINOR=${TOCATA_PEDAL_VERSION_MINOR}
        VERSION_SUBMINOR=${TOCATA_PEDAL_VERSION_SUBMINOR}
        )

target_link_libraries(${CUR_TARGET} PRIVATE 
        libremidi
        SDL2-static
        )
