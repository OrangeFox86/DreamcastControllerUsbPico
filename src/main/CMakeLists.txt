cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")

add_executable(main ${SRC})
pico_add_extra_outputs(main)
target_link_libraries(main
  PRIVATE
    hal
    coreLib
)
target_compile_options(main PRIVATE
  -Wall
  -Werror
  -O3
)

target_include_directories(main
  PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/main>"
    "${PROJECT_SOURCE_DIR}/inc")
