add_executable(ssfd ssfd.cpp ${ICON_RC})
target_link_libraries(ssfd ssf_framework)
set_property(TARGET ssfd PROPERTY FOLDER "Executables")
copy_certs(ssfd)

install(TARGETS ssfd RUNTIME DESTINATION bin)
