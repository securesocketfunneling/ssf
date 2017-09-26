cmake_minimum_required(VERSION 2.8)
include_directories(${OPENSSL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})

set(SERVER_NAME "ssfs")
set(SERVER_MAIN
      "${project_SRC_DIR}/core/server/main.cpp")

include_directories(
  ${OPENSSL_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS})

add_target(${SERVER_NAME}
  TYPE
    executable ${EXEC_FLAG} INSTALL
  FILES
    ${CORE_COMMAND_LINE_STANDARD_FILES}
    ${SSF_SOURCES}
    ${ICON_RC}
    ${SERVER_MAIN}
  LINKS
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    lib_ssf_network
  PREFIX_SKIP
    ${project_SRC_DIR})

install(TARGETS ssfs RUNTIME DESTINATION bin)

copy_certs(ssfs)
