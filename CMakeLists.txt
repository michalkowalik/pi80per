include(pico_sdk_import.cmake)

cmake_minimum_required(VERSION 3.13)

## the cmake output is annoying
cmake_policy(SET CMP0057 NEW)
set(CMAKE_C_STANDARD 11)

project(pitalker C ASM)

pico_sdk_init()


## project config

add_executable(pitalker main.c)

# pull in common dependencies
target_link_libraries(pitalker pico_stdlib)

# enable usb output, disable uart output
pico_enable_stdio_usb(pitalker 1)
pico_enable_stdio_uart(pitalker 0)

example_auto_set_url(pitalker)

