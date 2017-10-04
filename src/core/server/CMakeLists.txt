include_directories(${OPENSSL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})

add_target(ssfs
  TYPE
    executable ${EXEC_FLAG} INSTALL
  FILES
    main.cpp
    ${CORE_COMMAND_LINE_STANDARD_FILES}
    ${ICON_RC}
  LINKS
    lib_ssf_framework
  PREFIX_SKIP
    ${project_SRC_DIR})

install(TARGETS ssfs RUNTIME DESTINATION bin)

copy_certs(ssfs)
