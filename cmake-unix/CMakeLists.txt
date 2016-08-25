cmake_minimum_required(VERSION 2.8)

# --- Special settings to Unix platform
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

      add_library(linux_libc_funcs_version_downgrade
        "${project_SRC_DIR}/linux_compatibility/memcpy.cpp"
      )

      list(APPEND PLATFORM_SPECIFIC_LIB_DEP "linux_libc_funcs_version_downgrade")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--wrap=memcpy")
    endif()

  endif ()

endif(UNIX)
