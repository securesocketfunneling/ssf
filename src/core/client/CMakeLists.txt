cmake_minimum_required(VERSION 2.8)     

include_directories(
  ${OpenSSL_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS})

project("SSF Client")

# --- ssfc
set(CLIENT_NAME "ssfc")

set(CLIENT_FILES
    main.cpp)

add_target(${CLIENT_NAME}
  TYPE
    executable ${EXEC_FLAG} INSTALL
  FILES
    ${CLIENT_FILES}
    ${SSF_SOURCES}
    ${CORE_COMMAND_LINE_STANDARD_FILES}
    ${ICON_RC}
  LINKS
    ${Boost_LIBRARIES}
    ${OpenSSL_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
  PREFIX_SKIP
    ${project_SRC_DIR})

if (APPLE)
  set_source_files_properties(${ICON_RC} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  set_target_properties(${CLIENT_NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE ${ICON_NAME})
endif (APPLE)

# --- ssfcp
set(CLIENT_CP_NAME "ssfcp") 
set(CLIENT_CP_FILES
    main_cp.cpp) 
  
add_target(${CLIENT_CP_NAME}
  TYPE
    executable ${EXEC_FLAG} INSTALL
  FILES
    ${CLIENT_CP_FILES}
    ${SSF_SOURCES}
    ${CORE_COMMAND_LINE_COPY_FILES}
    ${ICON_RC}
  LINKS
    ${Boost_LIBRARIES}
    ${OpenSSL_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
  PREFIX_SKIP
    ${project_SRC_DIR})

if (APPLE)
  set_source_files_properties(${ICON_RC} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  set_target_properties(${CLIENT_CP_NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE ${ICON_NAME})
endif (APPLE)
