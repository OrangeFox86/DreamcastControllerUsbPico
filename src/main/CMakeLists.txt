cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

pico_sdk_init()
file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
add_executable(DreamcastControllerUsbPico
  ${SRC}
)
pico_generate_pio_header(DreamcastControllerUsbPico ${CMAKE_CURRENT_LIST_DIR}/maple.pio)
pico_add_extra_outputs(DreamcastControllerUsbPico)
target_link_libraries(DreamcastControllerUsbPico PRIVATE pico_stdlib pico_unique_id pico_multicore hardware_pio hardware_dma tinyusb_device tinyusb_board tinyusb_device_base)

target_include_directories(DreamcastControllerUsbPico PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_compile_options(DreamcastControllerUsbPico PRIVATE
    -Wall
    -Werror
    -O3
)


