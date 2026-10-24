# Little World Arcade - ESP-IDF project (ESP32-P4 / Waveshare 7B)
cmake_minimum_required(VERSION 3.16)

# Reuse the verified component tree at firmware/components (arcade_core, arcade_ui).
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Keep parity with the official 09_lvgl_demo_v9 build flags.
add_compile_options(-Wno-format)
add_compile_options(-Wno-ignored-qualifiers)
add_compile_options(-Wno-attributes)
add_compile_options(-Wno-unused-function)

project(little_world_arcade)
