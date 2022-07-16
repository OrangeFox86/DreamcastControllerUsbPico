cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")

add_executable(DreamcastControllerUsbPico ${SRC})
pico_add_extra_outputs(DreamcastControllerUsbPico)
target_link_libraries(DreamcastControllerUsbPico
  PRIVATE
    DreamcastControllerUsbPicoHal
    DreamcastControllerUsbPicoCore
)
target_compile_options(DreamcastControllerUsbPico PRIVATE
  -Wall
  -Werror
  -O3
)

target_include_directories(DreamcastControllerUsbPico PRIVATE "${CMAKE_CURRENT_LIST_DIR}" "${PROJECT_SOURCE_DIR}/inc")
