include_directories(${OPENSSL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})

add_target(ssfs
  TYPE
    executable ${EXEC_FLAG} INSTALL
  FILES
    ${CORE_COMMAND_LINE_STANDARD_FILES}
    ${SSF_SOURCES}
    ${ICON_RC}
    main.cpp
  LINKS
    ${Boost_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${PLATFORM_SPECIFIC_LIB_DEP}
    lib_ssf_network
  PREFIX_SKIP
    ${project_SRC_DIR})

install(TARGETS ssfs RUNTIME DESTINATION bin)

copy_certs(ssfs)
