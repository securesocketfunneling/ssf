set(TEST_CERT_FILES ${CMAKE_CURRENT_SOURCE_DIR}/certificate.crt
  ${CMAKE_CURRENT_SOURCE_DIR}/private.key
  ${CMAKE_CURRENT_SOURCE_DIR}/server.crt
  ${CMAKE_CURRENT_SOURCE_DIR}/server.key
  ${CMAKE_CURRENT_SOURCE_DIR}/dh4096.pem PARENT_SCOPE)
set(TEST_CERT_TRUSTED_FILES ${CMAKE_CURRENT_SOURCE_DIR}/trusted/ca.crt PARENT_SCOPE)

function(copy_certs _tgt)
  add_custom_command(TARGET ${_tgt} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${_tgt}>/certs/trusted
    COMMAND ${CMAKE_COMMAND} -E copy ${TEST_CERT_FILES} $<TARGET_FILE_DIR:${_tgt}>/certs
    COMMAND ${CMAKE_COMMAND} -E copy ${TEST_CERT_TRUSTED_FILES} $<TARGET_FILE_DIR:${_tgt}>/certs/trusted)
endfunction(copy_certs)

include(file2string)

file2string(test_client_cert certificate.crt client_cert.h)
file2string(test_client_key private.key client_key.h)
file2string(test_server_cert server.crt server_cert.h)
file2string(test_server_key server.key server_key.h)
file2string(test_server_dh_param dh4096.pem server_dh_param.h)
file2string(test_ca_cert trusted/ca.crt ca_cert.h)

add_library(test_certs INTERFACE)
target_include_directories(test_certs INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
