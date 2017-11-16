file(GLOB_RECURSE CONFIG_TEST_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/config_files/*.json")

file(MAKE_DIRECTORY config_files)
file(COPY ${CONFIG_TEST_FILES} DESTINATION config_files)

# --- Config tests

# --- Load config test
add_executable(load_config_tests EXCLUDE_FROM_ALL load_config_tests.cpp)
target_link_libraries(load_config_tests ssf_framework gtest)
set_property(TARGET load_config_tests PROPERTY FOLDER "Unit Tests/Config")
add_unit_test(load_config_tests)
