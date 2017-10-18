# --- Filesystem tests
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/filesystem)

add_executable(filesystem_tests EXCLUDE_FROM_ALL filesystem_tests.cpp)
target_link_libraries(filesystem_tests ssf_framework gtest)
set_property(TARGET filesystem_tests PROPERTY FOLDER "Unit Tests/Filesystem")
add_unit_test(filesystem_tests)
