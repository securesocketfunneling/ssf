include_directories(${OPENSSL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})

add_target(ssfc
  TYPE
    executable ${EXEC_FLAG} INSTALL
  FILES
    main.cpp
    ${CORE_COMMAND_LINE_STANDARD_FILES}
    ${ICON_RC}
  LINKS
    lib_ssf_framework
)

# --- ssfcp
add_target(ssfcp
  TYPE
    executable ${EXEC_FLAG} INSTALL
  FILES
    main_cp.cpp
    ${CORE_COMMAND_LINE_COPY_FILES}
    ${ICON_RC}
  LINKS
    lib_ssf_framework
)

install(TARGETS ssfc ssfcp RUNTIME DESTINATION bin)
copy_certs(ssfc)
