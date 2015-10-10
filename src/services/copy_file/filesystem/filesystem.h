#ifndef SSF_SERVICES_COPY_FILE_FILESYSTEM_FILESYSTEM_H_
#define SSF_SERVICES_COPY_FILE_FILESYSTEM_FILESYSTEM_H_

#include <cstdint>

#include <string>
#include <list>

namespace ssf {
namespace services {
namespace copy_file {
namespace filesystem {

class Filesystem {
 public:
  static std::list<std::string> Glob(const std::string& path);

  static std::string GetFilename(const std::string& path);

  static std::string GetParentPath(const std::string& path);
};

}  // filesystem
}  // copy_file
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_FILE_FILESYSTEM_FILESYSTEM_H_
