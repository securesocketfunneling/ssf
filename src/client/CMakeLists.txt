add_executable(ssf ssf.cpp ${ICON_RC})
target_link_libraries(ssf ssf_framework)
copy_certs(ssf)

add_executable(ssfcp ssfcp.cpp ${ICON_RC})
target_link_libraries(ssfcp ssf_framework)
copy_certs(ssfcp)

install(TARGETS ssf ssfcp RUNTIME DESTINATION bin)
