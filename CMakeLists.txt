cmake_minimum_required(VERSION 3.10)
project(MRTD_Automatisation_GPIB C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)

# Sources communes à toutes les plateformes
set(SOURCES_COMMON
    src/main.c
    src/hmi/hmi_ui.c
    src/vision/vision_mrtd.c
)

# Sources spécifiques à la plateforme
if(WIN32)
    set(SOURCES_PLATFORM
        src/gpib/gpib_win/gpib_win_ctrl.c
        src/gpib/gpib_win/gpib_win_read.c
        src/gpib/gpib_win/gpib_win_write.c
    )
else()
    set(SOURCES_PLATFORM
        src/gpib/gpib_linux/gpib_linux_ctrl.c
        src/gpib/gpib_linux/gpib_linux_read.c
        src/gpib/gpib_linux/gpib_linux_write.c
    )
endif()

add_executable(mon_app ${SOURCES_COMMON} ${SOURCES_PLATFORM})

target_include_directories(mon_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(mon_app PRIVATE
    PkgConfig::GTK3
)

# Lier libgpib uniquement sur Linux
if(UNIX)
    target_link_libraries(mon_app PRIVATE gpib)
endif()
