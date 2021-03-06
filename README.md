ARPC - An RPC framework that supports file descriptor passing
=============================================================

## Introduction

ARPC is an RPC framework that is heavily inspired by Google's Protobuf
and GRPC and aims to be compatible with its core C++ API.

Similar to GRPC/Protobuf, ARPC ships with a script called `aprotoc` that
can convert `.proto` files to C++ header files containing message and
service bindings. Where ARPC differs from GRPC/Protobuf is that it
provides support for attaching file descriptors to messages and sending
them to other processes on the system. It accomplishes this by making
use of [file descriptor passing](https://keithp.com/blogs/fd-passing/),
a feature that is available when making use of `AF_UNIX` sockets.

ARPC can be very useful for adding [privilege separation](https://en.wikipedia.org/wiki/Privilege_separation)
to your software. For example, a mail server could run with almost no
privileges, but still be able to deliver mail to user's mailboxes by
making use of an auxiliary process that hands out file descriptors to
mail spools stored on disk.

ARPC does not support any authentication and authorization, for the
reason that it is mainly intended to be used across UNIX sockets. It
also does not provide any support for concurrency and GRPC's
asynchronous API. Concurrency can be introduced by opening multiple
channels across separate UNIX sockets.

ARPC has been built on top of a serialization library called
[Argdata](https://github.com/NuxiNL/argdata), which in its turn has been
developed as hierarchical configuration file format for
[the CloudABI sandboxing framework](https://nuxi.nl/cloudabi/).

## Building ARPC

ARPC can be built natively using:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make

It can be compiled for CloudABI for i686 as follows:

    mkdir build
    cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-i686-cloudabi.cmake ..
    make

To install in a specific location, give `-DCMAKE_INSTALL_PREFIX` to CMake.
'make install' will then install the headers, library and `aprotoc` there.

## Using ARPC

ARPC should be easy to use if you already have some experience using
GRPC. Be sure to check out
[the GRPC C++ tutorial](http://www.grpc.io/docs/tutorials/basic/c.html)
to become familiar with the basics. Differences between GRPC and ARPC
worth mentioning:

- All of ARPC's definitions are stored in a single header file,
  `<arpc++/arpc++.h>` and are part of namespace `arpc`.
- In addition to the commonly used Protobuf datatypes (e.g., `int32`,
  `string`, `bool`), ARPC's `aprotoc` allows you to declare fields of
  type `fd`, which adds a field to the message of type
  `std::shared_ptr<FileDescriptor>`.
- ARPC servers and channels do not create UNIX sockets themselves. File
  descriptors of connected `AF_UNIX`, `SOCK_STREAM` sockets must be
  provided to `arpc::CreateChannel()` and `arpc::ServerBuilder`.
- [The unit tests](src/server_test.cc) also contain some examples of how
  to use ARPC.
