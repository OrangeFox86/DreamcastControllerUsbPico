cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

add_subdirectory(coreLib)

if(DREAMCAST_CONTROLLER_USB_PICO_TEST)
    add_subdirectory(test)
else()
    add_subdirectory(hal)
    add_subdirectory(main)
endif()
