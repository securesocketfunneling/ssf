if (UNIX)
  # --- Flags for compilation
  if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")

    set(CLANG_NO_BOOST_WARNINGS "-Wno-unneeded-internal-declaration -Wno-unused-variable")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++11 -stdlib=libc++ ${CLANG_NO_BOOST_WARNINGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

    set(PLATFORM_SPECIFIC_LIB_DEP "pthread")

  else()

    set(GCC_STATIC_LINK_FLAGS "-static-libstdc++ -static-libgcc")
    set(GCC_NO_SYMBOLS_FLAGS "-Wl,-s")
    set(GCC_NO_BOOST_WARNINGS "-Wno-long-long -Wno-unused-value -Wno-unused-local-typedefs")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -std=c++11 ${GCC_NO_BOOST_WARNINGS} ${GCC_STATIC_LINK_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -ggdb")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 ${GCC_NO_SYMBOLS_FLAGS}")

    set(PLATFORM_SPECIFIC_LIB_DEP "pthread" "rt"  ${CMAKE_DL_LIBS})

    if (${CMAKE_SIZEOF_VOID_P} EQUAL "8")
      # Downgrade version of libc function for 64 bits executables
      # Force libc version function and wrap it with linker
      #   * memcpy 2.14 -> 2.2.5
      list(APPEND PLATFORM_SPECIFIC_LIB_DEP linux_libc_funcs_version_downgrade)
    endif()

  endif ()
elseif (WIN32)
  include(MSVCStaticRuntime)
  include(HelpersIdeTarget)

  set(EXEC_FLAG "RUNTIME_STATIC")

  # --- Flags for compilation
  add_definitions(-D_WIN32_WINNT=0x0501)
  if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /wd4503")
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  endif(MSVC)
endif ()
