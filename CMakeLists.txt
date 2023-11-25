cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(pitalker C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()


## project config

add_executable(pitalker main.c)

# pull in common dependencies
target_link_libraries(pitalker pico_stdlib)
pico_add_extra_outputs(pitalker)

# enable usb output, disable uart output
pico_enable_stdio_usb(pitalker 1)
pico_enable_stdio_uart(pitalker 0)

