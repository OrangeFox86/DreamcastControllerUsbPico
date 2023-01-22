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
    hal-Usb-Hid
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





file(GLOB CLIENT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/client_volatile_mem.cpp")
add_executable(client-volatile-mem ${CLIENT_SRC})
pico_add_extra_outputs(client-volatile-mem)
target_link_libraries(client-volatile-mem
  PRIVATE
    pico_multicore
    hal-MapleBus
    hal-System
    pico_stdio_usb
    clientLib
)
target_compile_options(client-volatile-mem PRIVATE
  -Wall
  -Werror
  -O3
)

target_include_directories(client-volatile-mem
  PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/client>"
    "${PROJECT_SOURCE_DIR}/inc")





file(GLOB CLIENT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/client_pico_non_volatile_mem.cpp")
add_executable(client-pico-non-volatile-mem ${CLIENT_SRC})
pico_add_extra_outputs(client-pico-non-volatile-mem)
pico_set_binary_type(client-pico-non-volatile-mem no_flash)
target_link_libraries(client-pico-non-volatile-mem
  PRIVATE
    pico_multicore
    hal-MapleBus
    hal-System
    pico_stdio_usb
    clientLib
)
target_compile_options(client-pico-non-volatile-mem PRIVATE
  -Wall
  -Werror
  -O3
)

target_include_directories(client-pico-non-volatile-mem
  PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/client>"
    "${PROJECT_SOURCE_DIR}/inc")