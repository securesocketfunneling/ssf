#include "services/copy_file/filesystem/filesystem.h"

#include <glob.h>

namespace ssf {
namespace services {
namespace copy_file {
namespace filesystem {

std::list<std::string> Filesystem::Glob(const std::string& path) {
  std::list<std::string> result;

  glob_t glob_result;
  glob(path.c_str(), GLOB_TILDE, NULL, &glob_result);
  for (uint32_t i = 0; i < glob_result.gl_pathc; ++i) {
    result.push_back(std::string(glob_result.gl_pathv[i]));
  }
  globfree(&glob_result);

  return result;
}

}  // filesystem
}  // copy_file
}  // services
}  // ssf
