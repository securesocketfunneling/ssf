cmake_minimum_required(VERSION 2.8)
project("SSF")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 14)

set(SSF_VERSION_MAJOR 2)
set(SSF_VERSION_MINOR 2)
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

# --- Include CMake build system scripts
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# --- Include platform specific settings
if(UNIX)
  if(USE_STATIC_RUNTIME)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")
  endif(USE_STATIC_RUNTIME)
  if(DISABLE_RTTI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
  endif(DISABLE_RTTI)
  if(${CMAKE_SIZEOF_VOID_P} EQUAL "8")
    list(APPEND PLATFORM_SPECIFIC_LIB_DEP memcpy)
  endif()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
endif(UNIX)
if(MSVC)
  if(USE_STATIC_RUNTIME)
    set(EXEC_FLAG "RUNTIME_STATIC")
    foreach(fl CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
               CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE)
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
                        program_options
                        date_time
                        filesystem
                        regex
                        chrono
                        thread)
add_library(boost INTERFACE)
target_include_directories(boost INTERFACE ${Boost_INCLUDE_DIRS})
target_link_libraries(boost INTERFACE ${Boost_LIBRARIES})
if(UNIX)
  target_link_libraries(boost INTERFACE pthread)
endif(UNIX)
target_compile_definitions(boost INTERFACE BOOST_EXCEPTION_DISABLE)
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
endif()

# --- http-parser components
add_subdirectory(third_party/http-parser)
add_subdirectory(third_party/msgpack)

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

# --- Print summary
message(STATUS "Build configuration:")
message(STATUS "  Version: ${SSF_VERSION}")
message(STATUS "  Unit Tests: ${BUILD_UNIT_TESTS}")
message(STATUS "  Network Unit Tests: ${BUILD_NETWORK_UNIT_TESTS}")
message(STATUS "  TLS Disabled: ${DISABLE_TLS}")
message(STATUS "  Static Libraries: ${USE_STATIC_LIBS} (Boost: ${Boost_USE_STATIC_LIBS}, OpenSSL: ${OPENSSL_USE_STATIC_LIBS})")
message(STATUS "  Static Runtime: ${USE_STATIC_RUNTIME} (Boost: ${Boost_USE_STATIC_RUNTIME}, OpenSSL: ${OPENSSL_MSVC_STATIC_RT})")
message(STATUS "  RTTI disabled: ${DISABLE_RTTI}")
