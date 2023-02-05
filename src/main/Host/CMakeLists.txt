cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB HOST_SRC "${CMAKE_CURRENT_SOURCE_DIR}/host*.c*")
add_executable(host ${HOST_SRC})
pico_add_extra_outputs(host)
target_link_libraries(host
  PRIVATE
    pico_multicore
    hal-MapleBus
    hal-System
    hal-Usb-Client-Hid
    pico_stdio_usb
    hostLib
)
target_compile_options(host PRIVATE
  -Wall
  -Werror
  -O3
)

target_include_directories(host
  PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/host>"
    "${PROJECT_SOURCE_DIR}/inc")
