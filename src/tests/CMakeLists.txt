cmake_minimum_required(VERSION 2.8)

enable_testing()

include_directories(
  ${OpenSSL_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS})

include_directories(
  ${GTEST_ROOT_DIR}/include
)

add_subdirectory(commandline)
add_subdirectory(config)
add_subdirectory(network)
add_subdirectory(services)
