enable_testing()

add_library(tls_config_helper STATIC EXCLUDE_FROM_ALL
  tls_config_helper.h
  tls_config_helper.cpp
)
target_link_libraries(tls_config_helper PUBLIC
  ssf_framework
  test_certs
)

set_property(TARGET tls_config_helper PROPERTY FOLDER "Unit Tests")

add_subdirectory(commandline)
add_subdirectory(config)
add_subdirectory(network)
add_subdirectory(services)
