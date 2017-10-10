add_target(ssf
  TYPE
    executable ${EXEC_FLAG} INSTALL
  FILES
    ssf.cpp
    ${ICON_RC}
  LINKS
    ssf_framework
)
copy_certs(ssf)

add_target(ssfcp
  TYPE
    executable ${EXEC_FLAG} INSTALL
  FILES
    ssfcp.cpp
    ${ICON_RC}
  LINKS
    ssf_framework
)
copy_certs(ssfcp)

install(TARGETS ssf ssfcp RUNTIME DESTINATION bin)
