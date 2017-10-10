add_target(ssfd
  TYPE
    executable ${EXEC_FLAG} INSTALL
  FILES
    ssfd.cpp
    ${ICON_RC}
  LINKS
    ssf_framework
)
copy_certs(ssfd)

install(TARGETS ssfd RUNTIME DESTINATION bin)
