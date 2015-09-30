cmake_minimum_required(VERSION 2.8)

set(project_NAME "SSF_Client")
project(${project_NAME})

set(CLIENT_FILES
      main.cpp
      client.h
      client.ipp)

include_directories(
  ${OpenSSL_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS})

add_target(${project_NAME}
  TYPE
    executable ${EXEC_FLAG}
  FILES
    ${CLIENT_FILES}
    ${SSF_SOURCES}
    ${ICON_RC}
  PREFIX_SKIP
    ${project_SRC_DIR})

if (APPLE)
  set_source_files_properties(${ICON_RC} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  set_target_properties(${project_NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE ${ICON_NAME})
endif (APPLE)

target_link_libraries(${project_NAME} ${Boost_LIBRARIES} ${OpenSSL_LIBRARIES} ${PLATFORM_SPECIFIC_LIB_DEP})
