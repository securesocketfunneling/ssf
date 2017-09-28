add_library(iob_func STATIC EXCLUDE_FROM_ALL iob_func.cpp)
project_group(Compat iob_func)
add_library(linux_libc_funcs_version_downgrade STATIC EXCLUDE_FROM_ALL memcpy.cpp)
target_link_libraries(linux_libc_funcs_version_downgrade PUBLIC "-Wl,--wrap=memcpy")
