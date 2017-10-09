enable_testing()

add_target(tls_config_helper
  TYPE LIBRARY ${EXEC_FLAG}
  FILES
    tls_config_helper.h
    tls_config_helper.cpp
  LINKS
    ssf_framework
    test_certs
)

project_group("Unit Tests" tls_config_helper)

add_subdirectory(commandline)
add_subdirectory(config)
add_subdirectory(network)
add_subdirectory(services)
