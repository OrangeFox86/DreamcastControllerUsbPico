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





file(GLOB CLIENT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/client*.c*")
add_executable(client ${CLIENT_SRC})
pico_add_extra_outputs(client)
target_link_libraries(client
  PRIVATE
    pico_multicore
    hal-MapleBus
    hal-System
    pico_stdio_usb
    clientLib
)
target_compile_options(client PRIVATE
  -Wall
  -Werror
  -O3
)

target_include_directories(client
  PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/client>"
    "${PROJECT_SOURCE_DIR}/inc")