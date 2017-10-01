include_directories(${OPENSSL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})

add_target(ssfc
  TYPE
    executable ${EXEC_FLAG} INSTALL
  FILES
    ${SSF_SOURCES}
    ${CORE_COMMAND_LINE_STANDARD_FILES}
    ${ICON_RC}
    client_helper.h
    client_helper.cpp
    main.cpp
  LINKS
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    lib_ssf_network
  PREFIX_SKIP
    ${project_SRC_DIR})

add_target(ssfcp
  TYPE
    executable ${EXEC_FLAG} INSTALL
  FILES
    ${SSF_SOURCES}
    ${CORE_COMMAND_LINE_COPY_FILES}
    ${ICON_RC}
    client_helper.h
    client_helper.cpp
    main_cp.cpp
  LINKS
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    lib_ssf_network)

install(TARGETS ssfc ssfcp RUNTIME DESTINATION bin)

copy_certs(ssfc)
