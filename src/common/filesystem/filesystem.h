#ifndef SSF_COMMON_FILESYSTEM_FILESYSTEM_H_
#define SSF_COMMON_FILESYSTEM_FILESYSTEM_H_

#include <list>

#include <boost/system/error_code.hpp>

#include "common/filesystem/path.h"

namespace ssf {

class Filesystem {
 public:
  Filesystem();

  bool IsFile(const Path& path, boost::system::error_code& ec) const;

  bool IsDirectory(const Path& path, boost::system::error_code& ec) const;

  bool Exists(const Path& path, boost::system::error_code& ec) const;

  uint64_t GetFilesize(const Path& path, boost::system::error_code& ec) const;

  bool MakeDirectory(const Path& path, boost::system::error_code& ec) const;

  bool MakeDirectories(const Path& path, boost::system::error_code& ec) const;

  bool Remove(const Path& path, boost::system::error_code& ec) const;

  bool RemoveAll(const Path& path, boost::system::error_code& ec) const;

  std::list<Path> ListFiles(const Path& path, bool recursive,
                            boost::system::error_code& ec) const;

 private:
  template <class DirectoryIterator>
  std::list<Path> ListFilesImpl(const Path& path,
                                const std::string& filepath_pattern,
                                boost::system::error_code& ec) const;
};

}  // ssf

#endif  // SSF_COMMON_FILESYSTEM_FILESYSTEM_H_
