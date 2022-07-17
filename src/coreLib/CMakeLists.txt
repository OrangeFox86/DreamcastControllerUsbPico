cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")

add_library(coreLib STATIC ${SRC})
target_compile_options(coreLib PRIVATE
  -Wall
  -Werror
  -O3
)

target_include_directories(coreLib
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/core>"
    "${PROJECT_SOURCE_DIR}/inc")
