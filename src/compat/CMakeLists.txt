add_library(iob_func STATIC EXCLUDE_FROM_ALL iob_func.cpp)
set_property(TARGET iob_func PROPERTY FOLDER "Compat")

if(UNIX AND NOT APPLE)
  add_library(memcpy STATIC EXCLUDE_FROM_ALL memcpy.cpp)
  target_link_libraries(memcpy PUBLIC "-Wl,--wrap=memcpy")
  set_property(TARGET memcpy PROPERTY FOLDER "Compat")
endif()
