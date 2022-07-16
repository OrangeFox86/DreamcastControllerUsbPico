cmake_minimum_required(VERSION 3.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(GOOGLE_TEST_COMMIT_ID "4219e7254cb8c473f57f6065bd13d1520d7b708f")
include(FetchContent)
FetchContent_Declare(
  googletest
  # Specify the commit you depend on and update it regularly.
  URL "https://github.com/google/googletest/archive/${GOOGLE_TEST_COMMIT_ID}.zip"
)
FetchContent_MakeAvailable(googletest)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
add_executable(DreamcastControllerUsbPicoTest
  ${SRC}
)
target_link_libraries(DreamcastControllerUsbPicoTest PRIVATE gtest_main pthread DreamcastControllerUsbPicoCore)
target_include_directories(DreamcastControllerUsbPicoTest PRIVATE ${CMAKE_CURRENT_LIST_DIR})
enable_testing()
add_test(NAME all_tests COMMAND DreamcastControllerUsbPicoTest)
add_custom_target(test COMMAND ${CMAKE_CTEST_COMMAND}
                  DEPENDS DreamcastControllerUsbPicoTest)

