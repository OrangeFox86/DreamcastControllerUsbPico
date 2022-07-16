cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")

add_library(DreamcastControllerUsbPicoCore STATIC ${SRC})
target_compile_options(DreamcastControllerUsbPicoCore PRIVATE
  -Wall
  -Werror
  -O3
)

target_include_directories(DreamcastControllerUsbPicoCore
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Core>"
    "${PROJECT_SOURCE_DIR}/inc")
