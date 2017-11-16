enable_testing()

add_library(gtest STATIC EXCLUDE_FROM_ALL third_party/googletest/googletest/src/gtest-all.cc)
if(UNIX)
  target_link_libraries(gtest PUBLIC pthread)
endif(UNIX)
add_library(gtest_main STATIC EXCLUDE_FROM_ALL third_party/googletest/googletest/src/gtest_main.cc)
target_link_libraries(gtest_main gtest)
target_include_directories(gtest
  PRIVATE ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest
  PUBLIC ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include
)
target_include_directories(gtest_main
  PRIVATE ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest
  PUBLIC ${CMAKE_SOURCE_DIR}/third_party/googletest/googletest/include
)
set_property(TARGET gtest PROPERTY FOLDER "Unit Tests")
set_property(TARGET gtest_main PROPERTY FOLDER "Unit Tests")

add_custom_target(build_tests ALL)

function(add_unit_test targetname)
  message(STATUS "Registering unit test ${targetname}")

  add_dependencies(build_tests ${targetname})

  if (TARGET gtest_main)
    target_link_libraries(${targetname} gtest_main)
  endif()

  add_test(NAME ${targetname}
           COMMAND ${targetname} --gtest_output=xml:${CMAKE_BINARY_DIR}/gtest_reports/${targetname}.xml)
endfunction(add_unit_test)
