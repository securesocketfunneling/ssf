enable_testing()

add_library(tls_config_helper STATIC EXCLUDE_FROM_ALL
  tls_config_helper.h
  tls_config_helper.cpp
)
target_link_libraries(tls_config_helper PUBLIC
  ssf_framework
  test_certs
)

project_group("Unit Tests" tls_config_helper)

add_subdirectory(commandline)
add_subdirectory(config)
add_subdirectory(network)
add_subdirectory(services)
