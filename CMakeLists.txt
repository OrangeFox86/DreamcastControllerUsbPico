cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

# Have pico_sdk_import.cmake use the local SDK if it was pulled down; otherwise, use path at
# PICO_SDK_PATH environment variable as specified within pico_sdk_import.cmake
if (EXISTS "${CMAKE_SOURCE_DIR}/lib/pico-sdk/CMakeLists.txt")
  set(PICO_SDK_PATH "${CMAKE_SOURCE_DIR}/lib/pico-sdk/")
  message("Using local pico SDK at ('${PICO_SDK_PATH}')")
endif ()

if (NOT DEFINED PICO_SDK_PATH AND NOT DEFINED ENV{PICO_SDK_PATH})
  message(FATAL_ERROR "PICO_SDK_PATH not defined; either execute:\ngit submodule update --recursive --init\nor set PICO_SDK_PATH in environment")
endif ()

include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")

project(DreamcastControllerUsbPico)
pico_sdk_init()
file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
add_executable(DreamcastControllerUsbPico
  ${SRC}
)
pico_add_extra_outputs(DreamcastControllerUsbPico)
target_link_libraries(DreamcastControllerUsbPico PRIVATE pico_stdlib pico_multicore)

target_include_directories(DreamcastControllerUsbPico PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_compile_options(DreamcastControllerUsbPico PRIVATE
    -Wall
    -Werror
    -O3
)


