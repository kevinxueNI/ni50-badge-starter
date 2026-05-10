# ESP32-S3-Touch-LCD-4 Project Template
# Compatible with ESP-IDF v6.0.1 + LVGL 8.4 + Waveshare BSP v2.0.0
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Suppress warnings from BSP components incompatible with IDF v6.0.1
add_compile_options(-Wno-format -Wno-error=override-init -Wno-error=missing-braces)

project(esp32s3_lcd4_badge)
