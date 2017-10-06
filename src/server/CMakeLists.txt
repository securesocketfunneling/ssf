cmake_minimum_required(VERSION 2.8)

include_directories(
  ${OpenSSL_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS})

add_target(ssfd
  TYPE
    executable ${EXEC_FLAG} INSTALL
  FILES
    ssfd.cpp
    ${ICON_RC}
  LINKS
    lib_ssf_framework
  PREFIX_SKIP
    ${project_SRC_DIR})

copy_certs(ssfd)

install(
  TARGETS ssfd
  RUNTIME
  DESTINATION ssf/${project_EXEC_DIR_NAME}
)
