# ------------------------------------------------------------------------------
# This script makes all helpers for external packages available from caller.
#
# An external package is a package normally found through 'find_package()'
# CMake's function. The helpers, here, are to allow possibility to embed these
# external packages in the source tree of a project.
#
# The idea will be to store the source code of the (handled) external projects
# in the source of the main project and then to call the CMake's function
# 'find_package()' with the corresponding package's name.
#
# ------------------------------------------------------------------------------

#
# Update parent's search path for CMake Modules
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
file(GLOB _dirs_list *)
foreach(_dir ${_dirs_list})
  if(IS_DIRECTORY "${_dir}")
    if(EXISTS "${_dir}/CMakeLists.txt")
      add_subdirectory("${_dir}")
    else()
      list(APPEND CMAKE_MODULE_PATH "${_dir}")
    endif()
  endif()
endforeach()
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)
