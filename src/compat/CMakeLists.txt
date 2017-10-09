add_library(iob_func STATIC EXCLUDE_FROM_ALL iob_func.cpp)
project_group(Compat iob_func)
add_library(memcpy STATIC EXCLUDE_FROM_ALL memcpy.cpp)
target_link_libraries(memcpy PUBLIC "-Wl,--wrap=memcpy")
