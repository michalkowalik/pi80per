cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(pi80per C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
add_subdirectory(./no-OS-FatFs/src)

## project config

add_executable(
        pi80per
        main.c
        hw_config.c)

# pull in common dependencies
target_link_libraries(
        pi80per
        pico_stdlib
        no-OS-FatFS-SD-SDIO-SPI-RPi-Pico)
pico_add_extra_outputs(pi80per)

# enable usb output, disable uart output
pico_enable_stdio_usb(pi80per 1)
pico_enable_stdio_uart(pi80per 0)

