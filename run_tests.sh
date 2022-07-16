#!/bin/sh

BUILD_DIR="build-test"
GCC="/usr/bin/gcc"
GPP="/usr/bin/g++"

/usr/bin/cmake \
    --no-warn-unused-cli \
    -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE \
    -DCMAKE_BUILD_TYPE:STRING=Debug \
    -DCMAKE_C_COMPILER:FILEPATH=${GCC} \
    -DCMAKE_CXX_COMPILER:FILEPATH=${GPP} \
    -DDREAMCAST_CONTROLLER_USB_PICO_TEST:BOOL=TRUE \
    -S. \
    -B./${BUILD_DIR} \
    -G "Unix Makefiles" \

STATUS=$?
if [ $STATUS -ne 0 ]; then
    echo "CMake returned error exit code: ${STATUS}"
    echo "Exiting"
    exit $STATUS
fi

/usr/bin/cmake \
    --build ${BUILD_DIR} \
    --config Debug \
    --target test \
    -j 10 \

STATUS=$?
if [ $STATUS -ne 0 ]; then
    echo "CMake returned error exit code: ${STATUS}"
    echo "Exiting"
    exit $STATUS
fi
