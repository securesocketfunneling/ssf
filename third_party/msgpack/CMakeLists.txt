add_library(lib_msgpack_c INTERFACE)
target_include_directories(lib_msgpack_c INTERFACE 
  ${CMAKE_CURRENT_SOURCE_DIR}/msgpack-c/include)
