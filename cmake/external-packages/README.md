## Introduction
These helpers offer a way to embed external packages in a **CMake** based project.
They allow to find these packages that are not **CMake** aware to build during
the generation of the build environment.

## Available packages
* [Boost](boost/README.md)
* [Botan](botan/README.md)
* [OpenSSL](openssl/README.md)

## Generic Usage

### Syntax
```
find_package(<package_name> [<version>] REQUIRED
    [NO_COMPONENTS|[WITH_COMPONENTS] <components>]
    [FLAGS <flags>]
)
```
* `NO_COMPONENTS` indicates only package's headers are required. **NO**
components should be compiled when this flag is set
* `WITH_COMPONENTS` must be followed with a list of all additional components
to build

 > **Note**
 >
 > Both `NO_COMPONENTS` and `WITH_COMPONENTS` **cannot** be used at the same
 > time.

* `FLAGS` can be followed by any of the flags below:
 > **Note**
 >
 > Some additional flags may be available for package. Please refer to specific
 > documentations for complete list of flags.

 * `STATIC`: Flag to request static version of libraries
 * `RUNTIME_STATIC`: Flag to request libraries linkable to platforms' static
runtime library
 * `SOURCE_DIR` `<path>`: The absolute path to the package's source tree

### Preliminary
In order to allow the use of this script, several points must be done first:

#### CMakeCommon module
This module **must** be enabled in the project wishing to use the
`Find<package_name>.cmake` helper.

Please, refer to this [CMakeCommon](../../README.md#usage)'s help for more
information about how to enable this module.

#### Source package
The package's source tree must be available from the project's source tree.

If no [`SOURCE_DIR`](#generic-usage) path is provided from the
`find_package(...)` call, the script will automatically search the source tree
in the following prioritized paths:
1. `${<PACKAGE_NAME>_ROOT_DIR}`
2. `${CMAKE_SOURCE_DIR}/third_party`
3. `${PROJECT_SOURCE_DIR}/third_party`
4. `${CMAKE_CURRENT_SOURCE_DIR}/third_party`

> **Note**
>
> The name of the root directory that contains the package's source tree should
> be written in lower case. Otherwise, that name **must** match the name provided
> on `find_package(...)` call.

### Particular settings
For specific reasons it may be needed to force the location where the packages'
binary will be created or where the packages' source  tree should be expanded.

This can be achieved through the following variables:
* `EXTERNALS_BINARY_DIR`:
    If set, this variable forces the location where all packages' binaries must
    be created.
* `EXTERNALS_EXPAND_DIR`:
    If set, this variable forces the location where all packages' source archive
    must be expanded.
* `EXTERNALS_USE_RELATIVE_DIR`:
    If set, this variable indicates that binaries' and expanded archives'
    location must use a sub path matching the caller *CMakeLists.txt*'s one.

## Adding new package
**TODO**