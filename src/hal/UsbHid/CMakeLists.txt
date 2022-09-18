cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")

add_library(hal-UsbHid STATIC ${SRC})
target_link_libraries(hal-UsbHid
  PUBLIC
    pico_stdlib
    pico_unique_id
    tinyusb_device
    tinyusb_board
    tinyusb_device_base
)
target_compile_options(hal-UsbHid PRIVATE
  -Wall
  -Werror
  -O3
)

target_include_directories(hal-UsbHid
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/hal/Usb>"
    "${PROJECT_SOURCE_DIR}/inc")
