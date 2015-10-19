#include "services/copy_file/filesystem/filesystem.h"

#include <windows.h>

#include <algorithm>

#include <boost/filesystem.hpp>

namespace ssf {
namespace services {
namespace copy_file {
namespace filesystem {

std::list<std::string> Filesystem::Glob(const std::string& path) {
  std::list<std::string> result;
  HANDLE file_handle;
  WIN32_FIND_DATA data;
  auto parent_dir_path = Filesystem::GetParentPath(path);

  file_handle = FindFirstFile(path.c_str(), &data);
  if (file_handle != INVALID_HANDLE_VALUE) {
    do {
      result.push_back(parent_dir_path + '/' + data.cFileName);
    } while (FindNextFile(file_handle, &data) != 0);
  }

  return result;
}

}  // filesystem
}  // copy_file
}  // services
}  // ssf
