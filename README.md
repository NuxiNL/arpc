ARPC - Argdata RPC
==================

GRPC-like RPC library that supports file descriptor passing by using Argdata.

Can be built natively using:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make

For compilation within i686 CloudABI:

    mkdir build
    cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-i686-cloudabi.cmake ..
    make

To install in a specific location, give `-DCMAKE_INSTALL_PREFIX` to CMake.
'make install' will then install the headers, library and `aprotoc` there.
