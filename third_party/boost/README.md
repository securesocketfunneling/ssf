# Windows i386 executables only

This project needs to patch boost build system (add extra compilation flags).

Rename the patches directory for your boost version to ```patches```.

Start from a clean build directory. The patches will be applied at CMake project generation (```cmake ..```).

Renaming example:
* boost-1.61.0: ```patches-boost-1.61.0``` -> ```patches```