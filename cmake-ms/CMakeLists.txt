cmake_minimum_required(VERSION 2.8)

# --- Special settings to Windows platform
if (WIN32)
  include(MSVCStaticRuntime)
  include(HelpersIdeTarget)

  # --- Include windows impl
  
  # --- Filesystem globbing
  FILE(GLOB_RECURSE COPY_FILE_SERVICE_WINDOWS_FILES
      "${project_SRC_DIR}/services/copy_file/filesystem/windows/*.h"
      "${project_SRC_DIR}/services/copy_file/filesystem/windows/*.cpp")
      
  set(COPY_FILE_SERVICE_FILES
      ${COPY_FILE_SERVICE_FILES}
      ${COPY_FILE_SERVICE_WINDOWS_FILES})

  # --- Icon path
  set(ICON_RC "${project_IMG_DIR}/icon.rc")
  set(EXEC_FLAG "")

  # --- Boost platform requirements
  list(APPEND BOOST_PLATFORM_FLAGS "RUNTIME_STATIC")
  list(APPEND BOOST_PLATFORM_COMPONENTS "context" "coroutine")
    
  # --- OpenSSL platform requirements
  list(APPEND OPENSSL_PLATFORM_FLAGS "RUNTIME_STATIC")
  
  # --- Flags for compilation
  add_definitions(-D_WIN32_WINNT=0x0501)
  if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  endif(MSVC)
endif (WIN32)

