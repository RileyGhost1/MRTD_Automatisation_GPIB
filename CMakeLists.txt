cmake_minimum_required(VERSION 3.10)
project(MRTD_Automatisation_GPIB C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3    REQUIRED IMPORTED_TARGET gtk+-3.0)
pkg_check_modules(GMODULE REQUIRED IMPORTED_TARGET gmodule-2.0)

# ─── libharu ───────────────────────────────────────────────
find_path(HPDF_INCLUDE_DIR   NAMES hpdf.h)
find_library(HPDF_LIBRARY    NAMES hpdf)
find_package(PNG              REQUIRED)
find_package(ZLIB             REQUIRED)

if(NOT HPDF_INCLUDE_DIR OR NOT HPDF_LIBRARY)
    message(FATAL_ERROR "libharu (libhpdf-dev) introuvable. sudo apt install libhpdf-dev")
endif()
message(STATUS "libharu found: ${HPDF_LIBRARY}")
# ───────────────────────────────────────────────────────────

add_definitions(-DGLADE_PATH="${CMAKE_SOURCE_DIR}/src/hmi/gui.glade")

set(SOURCES_COMMON
    src/main.c
    src/gpib.c
    src/hmi/ui.c
    src/services.c
    src/parsing.c
    src/mrtd.c
    src/result_pdf.c          
)

add_executable(mon_app ${SOURCES_COMMON})

target_include_directories(mon_app PRIVATE
    ${GTK3_INCLUDE_DIRS}
    ${GMODULE_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pbplot
    ${HPDF_INCLUDE_DIR}        # ← headers hpdf.h
)

target_link_libraries(mon_app PRIVATE
    PkgConfig::GTK3
    PkgConfig::GMODULE
    ${HPDF_LIBRARY}            # ← libhpdf
    PNG::PNG                   # ← libpng (dépendance libharu)
    ZLIB::ZLIB                 # ← zlib  (dépendance libharu)
    m
)

if(UNIX)
    target_link_libraries(mon_app PRIVATE cjson gpib)
    target_link_options(mon_app PRIVATE -rdynamic)
endif()