cmake_minimum_required(VERSION 2.8)

# --- Copy test certs for server
set(project_BINARY_SERVER_DIR "${project_BINARY_DIR}/src/core/server")
set(project_BINARY_SERVER_CERT_DIR ${project_BINARY_SERVER_DIR}/certs)

file(MAKE_DIRECTORY ${project_BINARY_SERVER_CERT_DIR})
file(MAKE_DIRECTORY ${project_BINARY_SERVER_CERT_DIR}/trusted)

file(COPY ${SSF_CERT_TEST_ROOT_FILES}
     DESTINATION ${project_BINARY_SERVER_CERT_DIR})
file(COPY ${SSF_CERT_TEST_TRUSTED_FILES}
     DESTINATION ${project_BINARY_SERVER_CERT_DIR}/trusted)

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

install(
  TARGETS ssfs
  RUNTIME
  DESTINATION ssf/${project_EXEC_DIR_NAME}
)
