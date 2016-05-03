cmake_minimum_required(VERSION 2.8)

file(GLOB_RECURSE SSF_FRAMEWORK_COMMON_IO_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/io/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/io/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/io/*.ipp")

set(SSF_FRAMEWORK_COMMON_ERROR_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/error/error.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/error/error.ipp")

set(SSF_FRAMEWORK_COMMON_LOG_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/log/log.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/log/log.cpp")

file(GLOB_RECURSE SSF_FRAMEWORK_COMMON_NETWORK_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/network/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/network/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/network/*.ipp")

file(GLOB_RECURSE SSF_FRAMEWORK_COMMON_UTILS_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/utils/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/network/*.ipp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/network/*.cpp"
)

file(GLOB_RECURSE SSF_FRAMEWORK_LAYER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/layer/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/layer/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/layer/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/layer/*.ipp")

file(GLOB_RECURSE SSF_FRAMEWORK_INTERFACES_SYSTEM_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/system/*interface*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/system/*interface*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/system/*interface*.ipp")

file(GLOB_RECURSE SSF_FRAMEWORK_ROUTERS_SYSTEM_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/system/*router*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/system/*router*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ssf/system/*router*.ipp")

add_target("lib_ssf_network"
  TYPE
    library ${EXEC_FLAG} STATIC
  LINKS
    ${OpenSSL_LIBRARIES}
    ${Boost_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
  PREFIX_SKIP     .*/src
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
  FILES
    ${SSF_FRAMEWORK_COMMON_LOG_FILES}
    ${SSF_FRAMEWORK_COMMON_ERROR_FILES}
    ${SSF_FRAMEWORK_COMMON_IO_FILES}
    ${SSF_FRAMEWORK_COMMON_NETWORK_FILES}
    ${SSF_FRAMEWORK_COMMON_UTILS_FILES}
    ${SSF_FRAMEWORK_LAYER_FILES}
    ${SSF_FRAMEWORK_INTERFACES_SYSTEM_FILES}
    ${SSF_FRAMEWORK_ROUTERS_SYSTEM_FILES}
  INCLUDE_DIRECTORIES
    ${OpenSSL_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(lib_ssf_network
  INTERFACE
    ${OpenSSL_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if (BUILD_UNIT_TESTS)
  add_subdirectory(tests)
endif(BUILD_UNIT_TESTS)