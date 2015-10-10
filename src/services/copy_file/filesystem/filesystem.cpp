#include "services/copy_file/filesystem/filesystem.h"

namespace ssf {
namespace services {
namespace copy_file {
namespace filesystem {

std::string Filesystem::GetFilename(const std::string& path) {
  return path.substr(path.find_last_of("/\\") + 1);
}

std::string Filesystem::GetParentPath(const std::string& path) {
  return path.substr(0, path.find_last_of("/\\"));
}

}  // ssf
}  // services
}  // copy_file
}  // ssf
