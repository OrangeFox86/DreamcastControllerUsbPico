cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

if(DREAMCAST_CONTROLLER_USB_PICO_TEST)
    # Build for test
    add_subdirectory(test)
else()
    # Build main executable
    add_subdirectory(main)
endif()
