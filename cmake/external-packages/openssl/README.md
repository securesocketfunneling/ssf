## Introduction
This script helps CMake's `find_package(...)` feature to find and build the
**OpenSSL** package present in the project's source tree.

## Provided (ReadOnly) variables:
* `OpenSSL_FOUND`:
    Indicates whether the library has been found or not.
* `OpenSSL_VERSION`:
  Indicates the version of the library that has been found
* `OpenSSL_ROOT_DIR`:
    Absolute path where **OpenSSL**'s libraries and headers are located.
* `OpenSSL_INCLUDE_DIRS`
    Points to the **OpenSSL**'s include directories that should be passed to
    `include_directories(...)`.
* `OpenSSL_LIBRARIES`:
    Points specifically to the **OpenSSL**'s libraries that should be passed to
    `target_link_libraries(...)`.

## Usage

### Syntax
```
find_package(OpenSSL [<version>] REQUIRED
    [FLAGS <flags>]
)
```
* `FLAGS`, in addition to [generics](../README.md#syntax) can be any of the
following:
 * `EXTRA_FLAGS` `<flags>`: List of extra (i.e. raw) flags to provide to
**OpenSSL**'s configure script

> **Note**
>
> It is not possible to specify any additional components through the
> regular `WITH_COMPONENTS` directive. Use `EXTRA_FLAGS` instead.

### Example
* Find and compile **OpenSSL** in static link mode:

    ```cmake
    find_package(OpenSSL REQUIRED FLAGS STATIC)
    ```
* Find and compile **OpenSSL** in DLL link mode:

    ```cmake
    find_package(OpenSSL REQUIRED)
    ```
