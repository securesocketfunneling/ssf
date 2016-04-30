cmake_minimum_required(VERSION 2.8)

if(NOT DEP_BOOST_VERSION)
    set(DEP_BOOST_VERSION 1.55.0)
endif()

add_subdirectory(./client)
add_subdirectory(./server)
