cmake_minimum_required(VERSION 2.8)

include_directories(
  ${OpenSSL_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS})

# --- Copy test certs for client
set(project_BINARY_SERVER_DIR "${project_BINARY_DIR}/src/core/client")
set(project_BINARY_SERVER_CERT_DIR ${project_BINARY_SERVER_DIR}/certs)

file(MAKE_DIRECTORY ${project_BINARY_SERVER_CERT_DIR})
file(MAKE_DIRECTORY ${project_BINARY_SERVER_CERT_DIR}/trusted)

file(COPY ${SSF_CERT_TEST_ROOT_FILES}
     DESTINATION ${project_BINARY_SERVER_CERT_DIR})
file(COPY ${SSF_CERT_TEST_TRUSTED_FILES}
     DESTINATION ${project_BINARY_SERVER_CERT_DIR}/trusted)

# --- ssfc
set(CLIENT_NAME "ssfc")

set(CLIENT_MAIN
    client_helper.h
    client_helper.cpp
    main.cpp)

add_target(${CLIENT_NAME}
  TYPE
    executable ${EXEC_FLAG} INSTALL
  FILES
    ${SSF_SOURCES}
    ${CORE_COMMAND_LINE_STANDARD_FILES}
    ${ICON_RC}
    ${CLIENT_MAIN}
  LINKS
    ${Boost_LIBRARIES}
    ${OpenSSL_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    lib_ssf_network
  PREFIX_SKIP
    ${project_SRC_DIR})

# --- ssfcp
set(CLIENT_CP_NAME "ssfcp")
set(CLIENT_CP_MAIN
    client_helper.h
    client_helper.cpp
    main_cp.cpp)

add_target(${CLIENT_CP_NAME}
  TYPE
    executable ${EXEC_FLAG} INSTALL
  FILES
    ${SSF_SOURCES}
    ${CORE_COMMAND_LINE_COPY_FILES}
    ${ICON_RC}
    ${CLIENT_CP_MAIN}
  LINKS
    ${Boost_LIBRARIES}
    ${OpenSSL_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    lib_ssf_network
  PREFIX_SKIP
    ${project_SRC_DIR})

install(
  TARGETS ssfc ssfcp
  RUNTIME
  DESTINATION ssf/${project_EXEC_DIR_NAME}
)
