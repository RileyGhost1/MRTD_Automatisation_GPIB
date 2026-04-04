cmake_minimum_required(VERSION 3.10)
project(MRTD_Automatisation_GPIB C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3    REQUIRED IMPORTED_TARGET gtk+-3.0)
pkg_check_modules(GMODULE REQUIRED IMPORTED_TARGET gmodule-2.0)

add_definitions(-DGLADE_PATH="${CMAKE_SOURCE_DIR}/src/hmi/gui.glade")

# Sources toujours compilées
set(SOURCES_COMMON
    src/main.c
    src/gpib/gpib_ctrl.c
    src/hmi/hmi_ui.c
    src/job/job_manager.c
    src/vision/vision_mrtd.c
)

# UN SEUL add_executable
add_executable(mon_app ${SOURCES_COMMON} ${SOURCES_PLATFORM})

target_include_directories(mon_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(mon_app PRIVATE
    PkgConfig::GTK3
    PkgConfig::GMODULE
)

if(UNIX)
    target_link_libraries(mon_app PRIVATE gpib)
    target_link_options(mon_app PRIVATE -rdynamic)
endif()
