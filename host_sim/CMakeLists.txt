cmake_minimum_required(VERSION 3.16)
project(arcade_sim C)

set(CMAKE_C_STANDARD 11)

# LVGL desktop build using our simulator configuration.
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/lv_conf.h" CACHE PATH "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
set(LV_BUILD_LVGL_H_SIMPLE_INCLUDE ON CACHE BOOL "" FORCE)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../firmware/managed_components/lvgl__lvgl" lvgl_build EXCLUDE_FROM_ALL)

file(GLOB UI_SOURCES "${CMAKE_CURRENT_LIST_DIR}/../firmware/components/arcade_ui/src/*.c")
file(GLOB FONT_SOURCES "${CMAKE_CURRENT_LIST_DIR}/../firmware/components/arcade_ui/fonts/*.c")
file(GLOB CORE_SOURCES "${CMAKE_CURRENT_LIST_DIR}/../firmware/components/arcade_core/src/*.c")

add_executable(arcade_sim
    sim_main.c
    esp_stubs.c
    ${UI_SOURCES}
    ${FONT_SOURCES}
    ${CORE_SOURCES})

target_include_directories(arcade_sim PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/stubs"
    "${CMAKE_CURRENT_LIST_DIR}/../firmware/components/arcade_ui/include"
    "${CMAKE_CURRENT_LIST_DIR}/../firmware/components/arcade_ui/src"
    "${CMAKE_CURRENT_LIST_DIR}/../firmware/components/arcade_core/include"
    "${CMAKE_CURRENT_LIST_DIR}/../firmware/managed_components/lvgl__lvgl")

target_compile_definitions(arcade_sim PRIVATE LV_CONF_INCLUDE_SIMPLE LV_LVGL_H_INCLUDE_SIMPLE)

if(MSVC)
    # Chinese UI strings are UTF-8 in the sources; without /utf-8 MSVC assumes the ANSI codepage.
    target_compile_options(arcade_sim PRIVATE /utf-8)
endif()

target_link_libraries(arcade_sim PRIVATE lvgl)
