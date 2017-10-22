Cross compiling SSF
===================

Toolchain
---------

First you need to obtain the toolchain for the target architecture. Choosing
or building the right toolchain is outside of the scope of this document.

In the rest of this document we will use the arm-linux-gnueabihf toolchain
which can be used to cross-compile applications for the Raspberry Pi 2/3
platform. On Debian/Ubuntu you can install it using the package manager.

```
apt-get install g++-arm-linux-gnueabihf
```

Building dependencies
---------------------

SSF depends on Boost and OpenSSL, these need to be cross-compiled separately.
To make your life easier, use the scripts provided in `builddeps/`.

```
CROSS_PREFIX=arm-linux-gnueabihf- /path/to/ssf/source/builddeps/build_openssl.sh /path/to/openssl/prefix/
CROSS_PREFIX=arm-linux-gnueabihf- /path/to/ssf/source/builddeps/build_boost.sh /path/to/boost/prefix
```

Building SSF
------------

To cross-compile SSF, you need to tell CMake how to cross-compile. Use the
`cmake/arm-linux-gnueabihf.cmake` configuration file, or customize it
according to your needs.

The toolchain configuration file needs to look like this (adjust the toolchain
tuple `arm-linux-gnueabihf`):

```
include(CMakeForceCompiler)
set(CMAKE_SYSTEM_NAME Linux)
CMAKE_FORCE_C_COMPILER(arm-linux-gnueabihf-gcc GNU)
CMAKE_FORCE_CXX_COMPILER(arm-linux-gnueabihf-g++ GNU)
set(CMAKE_SIZEOF_VOID_P 4)
```

**NOTE**: Due to an apparent bug in CMake depending on which version of GCC
the toolchain is using, CMake can choose to ignore the CMAKE_CXX_STANDARD
variable. You might want to add the following line to the toolchain
configuration file:

```
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
```

Then, from the build directory, invoke cmake specifying your toolchain
configuration file using the `-DCMAKE_TOOLCHAIN_FILE` parameter:

```
cmake -DUSE_STATIC_LIBS=ON -DCMAKE_TOOLCHAIN_FILE=arm-linux-gnueabihf.cmake -DBOOST_ROOT=/path/to/boost/prefix -DOPENSSL_ROOT_DIR=/path/to/openssl/prefix /path/to/ssf/source
```

Then proceed to build SSF:

```
make
```
