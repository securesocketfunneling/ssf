cmake_minimum_required(VERSION 2.8)

set(project_NAME "ssfc")
project(${project_NAME})

set(CLIENT_FILES
    main.cpp)
 
set(CLIENT_CP_PROJECT_NAME "ssfcp") 
set(CLIENT_CP_FILES
    main_cp.cpp)      

include_directories(
  ${OpenSSL_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS})

# --- SSF_Client
add_target(${project_NAME}
  TYPE
    executable ${EXEC_FLAG}
  FILES
    ${CLIENT_FILES}
    ${SSF_SOURCES}
    ${CORE_COMMAND_LINE_STANDARD_FILES}
    ${ICON_RC}
  PREFIX_SKIP
    ${project_SRC_DIR})

if (APPLE)
  set_source_files_properties(${ICON_RC} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  set_target_properties(${project_NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE ${ICON_NAME})
endif (APPLE)

target_link_libraries(${project_NAME} ${Boost_LIBRARIES} ${OpenSSL_LIBRARIES} ${PLATFORM_SPECIFIC_LIB_DEP})

# --- SSF_Client_cp
add_target(${CLIENT_CP_PROJECT_NAME}
  TYPE
    executable ${EXEC_FLAG}
  FILES
    ${CLIENT_CP_FILES}
    ${SSF_SOURCES}
    ${CORE_COMMAND_LINE_COPY_FILES}
    ${ICON_RC}
  PREFIX_SKIP
    ${project_SRC_DIR})

if (APPLE)
  set_source_files_properties(${ICON_RC} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  set_target_properties(${CLIENT_CP_PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_ICON_FILE ${ICON_NAME})
endif (APPLE)

target_link_libraries(${CLIENT_CP_PROJECT_NAME} ${Boost_LIBRARIES} ${OpenSSL_LIBRARIES} ${PLATFORM_SPECIFIC_LIB_DEP})
