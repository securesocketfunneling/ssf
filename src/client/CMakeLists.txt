cmake_minimum_required(VERSION 2.8)

# --- ssf
add_target("ssf"
  TYPE
    executable ${EXEC_FLAG} INSTALL
  FILES
    ssf.cpp
    ${ICON_RC}
  LINKS
    ssf_framework
)
copy_certs(ssf)

# --- ssfcp
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

# --- install directive
install(
  TARGETS ssf ssfcp
  RUNTIME
  DESTINATION ssf/${project_EXEC_DIR_NAME}
)
