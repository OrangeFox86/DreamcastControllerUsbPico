cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")

add_library(hal STATIC ${SRC})
pico_generate_pio_header(hal ${CMAKE_CURRENT_LIST_DIR}/maple.pio)
target_link_libraries(hal
  PUBLIC
    pico_stdlib
    pico_unique_id
    pico_multicore
    hardware_pio
    hardware_dma
    tinyusb_device
    tinyusb_board
    tinyusb_device_base
)
target_compile_options(hal PRIVATE
  -Wall
  -Werror
  -O3
)

target_include_directories(hal
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/hal>"
    "${PROJECT_SOURCE_DIR}/inc")
