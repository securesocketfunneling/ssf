cmake_minimum_required(VERSION 3.0)
project("SSF")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 14)

set(SSF_VERSION_MAJOR 3)
set(SSF_VERSION_MINOR 0)
set(SSF_VERSION_FIX 0)
set(SSF_VERSION_CIRCUIT 2)
set(SSF_VERSION_TRANSPORT 2)

set(SSF_VERSION "${SSF_VERSION_MAJOR}.${SSF_VERSION_MINOR}.${SSF_VERSION_FIX}")

# --- Options
include(CMakeDependentOption)
option(BUILD_UNIT_TESTS "Build SSF unit tests" OFF)
option(BUILD_NETWORK_UNIT_TESTS "Build SSF network layers unit tests" OFF)
option(DISABLE_TLS "Disable TLS" OFF)
option(USE_STATIC_LIBS "Link binaries statically against libraries" OFF)
CMAKE_DEPENDENT_OPTION(USE_STATIC_RUNTIME "Link statically against the C++ runtime libraries"
                       ON "USE_STATIC_LIBS" OFF)
option(DISABLE_RTTI "Disable C++ Runtime Type Information" OFF)
option(DISABLE_LOGS "Disable logs" OFF)
if (UNIX)
option(ENABLE_SYSLOG "Use syslog collector" ON)
endif (UNIX)

# --- Set default CMAKE_BUILD_TYPE if none is provided
# (Only relevant for single-configuration generators)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# --- Include CMake build system scripts
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# --- Include platform specific settings
if(CMAKE_COMPILER_IS_GNUCXX)
  if(USE_STATIC_RUNTIME)
    list(APPEND CMAKE_EXE_LINKER_FLAGS "-static-libstdc++ -static-libgcc")
    list(APPEND PLATFORM_LIBS libstdc++.a)
  endif(USE_STATIC_RUNTIME)
  if(DISABLE_RTTI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
  endif(DISABLE_RTTI)
  if((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.7) AND (${CMAKE_SIZEOF_VOID_P} EQUAL "8"))
    list(APPEND PLATFORM_LIBS memcpy)
  endif()

  list(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE "-s -Wl,--no-export-dynamic")
  list(APPEND CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "-s -Wl,--no-export-dynamic")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_DEBUG} -Og -ggdb")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
  set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os")
endif()
if(MSVC)
  if(USE_STATIC_RUNTIME)
    set(EXEC_FLAG "RUNTIME_STATIC")
    foreach(fl CMAKE_C_FLAGS CMAKE_CXX_FLAGS
               CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG
               CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO
               CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE
               CMAKE_C_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_MINSIZEREL)
      string(REPLACE "/MD" "/MT" ${fl} "${${fl}}")
    endforeach()
  endif(USE_STATIC_RUNTIME)
  if(DISABLE_RTTI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
  endif(DISABLE_RTTI)
  add_definitions(-D_WIN32_WINNT=0x0501)
  # Increase the number of sections that an object file can contain
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
  # Disable warning: 'identifier': decorated name length exceeded, name was truncated
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4503")
  # Disable warning C4996: security advice
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

# --- Boost components
# Force linking boost statically on Visual Studio
if (MSVC)
  set(Boost_USE_STATIC_LIBS ON)
else()
  set(Boost_USE_STATIC_LIBS ${USE_STATIC_LIBS})
endif()

set(Boost_USE_STATIC_RUNTIME ${USE_STATIC_RUNTIME})
set(Boost_USE_MULTITHREADED ON)
find_package(Boost REQUIRED
             COMPONENTS system
                        filesystem
                        chrono
                        thread
                        date_time)
add_library(boost INTERFACE)
target_include_directories(boost INTERFACE ${Boost_INCLUDE_DIRS})
target_link_libraries(boost INTERFACE ${Boost_LIBRARIES})
target_compile_definitions(boost INTERFACE
  BOOST_EXCEPTION_DISABLE
  BOOST_ASIO_DISABLE_BOOST_REGEX
  BOOST_ASIO_DISABLE_BOOST_DATE_TIME)

if(UNIX)
  target_link_libraries(boost INTERFACE pthread)
endif(UNIX)
if (DISABLE_RTTI)
  target_compile_definitions(boost INTERFACE BOOST_NO_RTTI BOOST_NO_TYPEID)
endif(DISABLE_RTTI)

# --- OpenSSL components
set(OPENSSL_USE_STATIC_LIBS ${USE_STATIC_LIBS})
set(OPENSSL_MSVC_STATIC_RT ${USE_STATIC_RUNTIME})
find_package(OpenSSL REQUIRED)
add_library(ssl INTERFACE)
target_include_directories(ssl INTERFACE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(ssl INTERFACE ${OPENSSL_LIBRARIES})
if(OPENSSL_USE_STATIC_LIBS AND MSVC_VERSION)
  include(CheckFunctionExists)
  CHECK_FUNCTION_EXISTS("_snprintf" HAVE_SNPRINTF)
  if(NOT HAVE_SNPRINTF)
    target_link_libraries(ssl INTERFACE legacy_stdio_definitions.lib)
  endif(NOT HAVE_SNPRINTF)
  CHECK_FUNCTION_EXISTS("__iob_func" HAVE_IOB_FUNC)
  if(NOT HAVE_IOB_FUNC)
    target_link_libraries(ssl INTERFACE iob_func)
  endif(NOT HAVE_IOB_FUNC)
  target_compile_definitions(ssl INTERFACE
    OPENSSL_NO_ERR)
endif()
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")

# --- http-parser components
add_subdirectory(third_party/http-parser)
add_subdirectory(third_party/msgpack)
add_subdirectory(third_party/spdlog)
add_subdirectory(third_party/json)

# --- cxxopts
add_subdirectory(third_party/cxxopts EXCLUDE_FROM_ALL)

# --- Test certs
add_subdirectory(certs)

# --- Unit tests
if (BUILD_UNIT_TESTS OR BUILD_FRAMEWORK_UNIT_TESTS)
  include(unit_tests)
endif (BUILD_UNIT_TESTS OR BUILD_FRAMEWORK_UNIT_TESTS)

# -- Images
add_subdirectory(img)

# --- Add sources
add_subdirectory(src)

install(FILES ${TEST_CERT_FILES} DESTINATION bin/certs)
install(FILES ${TEST_CERT_TRUSTED_FILES} DESTINATION bin/certs/trusted)

# --- Print summary
message(STATUS "Build configuration:")
message(STATUS "  Version: ${SSF_VERSION}")
message(STATUS "  Unit Tests: ${BUILD_UNIT_TESTS}")
message(STATUS "  Network Unit Tests: ${BUILD_NETWORK_UNIT_TESTS}")
message(STATUS "  TLS Disabled: ${DISABLE_TLS}")
message(STATUS "  Static Libraries: ${USE_STATIC_LIBS} (Boost: ${Boost_USE_STATIC_LIBS}, OpenSSL: ${OPENSSL_USE_STATIC_LIBS})")
message(STATUS "  Static Runtime: ${USE_STATIC_RUNTIME} (Boost: ${Boost_USE_STATIC_RUNTIME}, OpenSSL: ${OPENSSL_MSVC_STATIC_RT})")
message(STATUS "  RTTI disabled: ${DISABLE_RTTI}")
message(STATUS "  Logs disabled: ${DISABLE_LOGS}")
if (UNIX)
message(STATUS "  Syslog collector enabled: ${ENABLE_SYSLOG}")
endif (UNIX)
