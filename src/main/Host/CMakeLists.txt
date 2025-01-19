cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB HOST_SRC "${CMAKE_CURRENT_SOURCE_DIR}/host*.c*")
add_executable(host-1p-flycast ${HOST_SRC})
pico_add_extra_outputs(host-1p-flycast)
target_link_libraries(host-1p-flycast
  PRIVATE
    pico_multicore
    hal-MapleBus
    hal-System
    hal-Usb-Client-Hid
    pico_stdio_usb
    hostLib
)
target_compile_options(host-1p-flycast PRIVATE
  -Wall
  -Werror
  -O3
)
target_compile_definitions(host-1p-flycast PUBLIC SELECTED_NUMBER_OF_DEVICES=1)

target_include_directories(host-1p-flycast
  PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/host-1p-flycast>"
    "${PROJECT_SOURCE_DIR}/inc")
