cmake_minimum_required(VERSION 2.8)

project("SSF Server")

set(SERVER_NAME "ssfs")
set(SERVER_MAIN
      "${project_SRC_DIR}/core/server/main.cpp")

include_directories(
  ${OpenSSL_INCLUDE_DIRS}
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
    ${OpenSSL_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
  PREFIX_SKIP
    ${project_SRC_DIR})

if (APPLE)
  set_source_files_properties(${ICON_RC} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  set_target_properties(${SERVER_NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE ${ICON_NAME})
endif (APPLE)