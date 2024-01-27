cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(pis C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()


## project config

add_executable(pis main.c)

# pull in common dependencies
target_link_libraries(pis pico_stdlib)
pico_add_extra_outputs(pis)

# enable usb output, disable uart output
pico_enable_stdio_usb(pis 1)
pico_enable_stdio_uart(pis 0)

