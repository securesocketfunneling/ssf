## Introduction
This script helps CMake's `find_package(...)` feature to find and build the
**Boost** package present in the project's source tree.

## Provided (ReadOnly) variables:
* `Boost_FOUND`:
    Indicates whether the library has been found or not.
* `Boost_VERSION`:
    Indicates the version of the library that has been found
* `Boost_INCLUDE_DIRS`:
    Points to the **Boost**'s include directories that should be passed to
    `include_directories(...)`.
* `Boost_LIBRARIES`:
    Points specifically to the **Boost**'s libraries that should be passed to
    `target_link_libraries(...)`.
* `Boost_LIBRARY_DIRS`:
    Points specifically to the **Boost**'s libraries that should be passed to 
    `link_directories(...)`.
    This may be needed in case of use of auto import pragma feature.

## Usage

### Syntax
```
find_package(Boost [<version>] REQUIRED
    [NO_COMPONENTS|[WITH_COMPONENTS] <components>]
    [FLAGS <flags>]
)
```
* `FLAGS`, in addition to [generics](../README.md#syntax) can be any of the
following:
 * `EXTRA_FLAGS` `<flags>`: List of extra (i.e. raw) flags to provide to
**Boost**'s build tool (i.e. **bjam**)

### Example
* Find **Boost** but no components are required:

    ```cmake
    find_package(Boost REQUIRED NO_COMPONENTS)
    ```
* Find and compile **Boost** in static link mode with **ALL** components:

    ```cmake
    find_package(Boost REQUIRED FLAGS STATIC)
    ```
* Find and compile **Boost** in DLL link mode. Only `program_options` components
is required:

    ```cmake
    find_package(Boost REQUIRED
      WITH_COMPONENTS program_options
    )
    ```
