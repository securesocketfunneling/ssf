Building SSF for Windows
========================

Get build dependencies
----------------------

SSF depends on Boost and OpenSSL libraries. You can choose to build
dependencies yourself, or install prebuilt library packages.

### Using prebuilt packages

Obtain installers for the libraries and install them:

* Boost 1.65.1:

Install `boost_1_65_1-msvc-14.1-32.exe` (32-bit) or
`boost_1_65_1-msvc-14.1-64.exe` (64-bit) from
http://sourceforge.net/projects/boost/files/boost-binaries.
Replace `msvc-14.1` with your version of Visual Studio.

You will then need to point the cmake `BOOST_ROOT` variable to the location
of the boost install (By default: `C:\local\boost_1_65_1`).

* OpenSSL 1.0.2m:

Install `Win32OpenSSL-1_0_2m.exe` (32-bit) or `Win64OpenSSL-1_0_2m.exe` (64-bit)
from https://slproweb.com/products/Win32OpenSSL.html

*NOTE*: OpenSSL versions 1.1 is currently incompatible with Boost.
The latest compatible prebuilt version of OpenSSL is 1.0.2l.

If you have not installed OpenSSL in the default directory (`C:\OpenSSL-Win32`
for `C:\OpenSSL-Win64`), you will need to point the cmake variable
`OPENSSL_ROOT_DIR` to the install location.

### Building dependencies yourself

Boost and OpenSSL dependencies will need to be built manually. Convenience
scripts for automatically building these can be found in the `builddeps`
directory.

7-Zip is required to decompress the source tarballs.

First prepare a build directory, download boost and openssl source and place
them inside the build directory:

```
C:\Users\user> mkdir C:\build
C:\Users\user> cd C:\build
```

Boost 1.65.1 can be downloaded from https://dl.bintray.com/boostorg/release/1.65.1/source/boost_1_65_1.tar.bz2
and OpenSSL 1.0.2m from https://www.openssl.org/source/openssl-1.0.2m.tar.gz

### Building boost

Build Boost using `build_boost.bat`

```
C:\build> C:\path_to_ssf_source\builddeps\build_boost.bat C:\Users\user\Downloads\boost_1_65_1.tar.bz2 1_65_1 32 C:\boost
```

Pass `32` for 32-bit or `64` for 64-bit builds. Boost headers and
libraries will be installed in `C:\boost`

**NOTE**: `build_boost.bat` will build a version of Boost without C++ RTTI
support, when generating the project files prior to building SSF, make sure
`DISABLE_RTTI` is set to `ON`.

**NOTE**: `build_boost.bat` will only build static/runtime static versions of
the boost libraries.

### Building OpenSSL

A Perl distribution is required for building OpenSSL, you can get
Strawberry Perl 5.26.0.2 from http://strawberryperl.com.

The netwide assembler is also required for building OpenSSL. Grab and
install NASM from http://www.nasm.us/.

Make sure `perl.exe` and `nasm.exe` can be found in your environment before
running the following commands (adjust `Path` if needed).

```
C:\build> C:\path_to_ssf_source\builddeps\build_openssl.bat C:\Users\user\Downloads\openssl-1.0.2m.tar.gz 1.0.2m 32 C:\openssl
```

Pass `32` for 32-bit or `64` for 64-bit builds. OpenSSL headers and
libraries will be installed in `C:\openssl`

**NOTE**: `build_openssl.bat` will only build the static/runtime static version
of OpenSSL.

Building SSF
------------

SSF requires CMake and Visual Studio 2017 (2015 should work as well)

If you obtained the source for the git repository, make sure the submodules
are checked out:

```
C:\path_to_ssf_source> git submodule update --init
```

Generate the project files with CMake in your build directory. Point the
`BOOST_ROOT` and `OPENSSL_ROOT_DIR` variables to the correct location
(or leave empty for default settings).

```
C:\build> cmake C:\path_to_ssf_source -DBOOST_ROOT=C:\local\boost_1_65_1  -DOPENSSL_ROOT_DIR=C:\OpenSSL-Win32 -DUSE_STATIC_LIBS=ON
```

Various parameters can be customized when generating the project files:

* `USE_STATIC_LIBS`: `ON` or `OFF` to enable/disable linking statically against
boost and OpenSSL. On windows, you want this to be `ON` in most of the cases.
* `USE_STATIC_RUNTIME`: `ON` or `OFF` to enable/disable linking statically
against the C++ runtime library (msvcrt). This is set automatically to the same
value as `USE_STATIC_LIBS`.
* `BUILD_UNIT_TESTS`: `ON` or `OFF` to enable/disable building SSF unit tests.
* `DISABLE_RTTI`: `ON` or `OFF` to disable/enable C++ Run-Time Type Information.
RTTI is enabled by default. Only disable RTTI if boost libraries have been built
without RTTI.
* `DISABLE_TLS`: `ON` or `OFF` to disable/enable TLS layer. Network traffic will
use raw TCP and be left unsecured. Provided for testing purpose only.

Proceed to build SSF:

```
C:\build> cmake --build . --config Release
```

Binaries are located in: `src\client\Release\ssf.exe`, `src\client\Release\ssfcp.exe`
and `src\server\Release\ssfd.exe`.

Replace `Release` with `Debug` for debug binaries.

XP Compatibility
----------------

To build XP compatible binaries, the correct Visual Studio toolset needs to be
passed to cmake when generating project files. Note that the toolset needs
to be present in your Visual Studio installation. Here we use the `v141_xp`
toolset:

```
C:\build> cmake -G "Visual Studio 15 2017" -T v141_xp C:\path_to_ssf_source
```
