# ------------------------------------------------------------------------------
# This script makes all CMakeCommon's helpers available for caller
#
# For more information about available helpers, please have a look to the
# script packages in this source tree.
#
# Usage:
#   add_subdirectory("/path/to/cmake-common")
#
# ------------------------------------------------------------------------------

# ==============================================================================
# =============== U p d a t e   c m a k e   m o d u l e   p a t h ============== 
# ==============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
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
