#ifndef SSF_COMMON_FILESYSTEM_PATH_H_
#define SSF_COMMON_FILESYSTEM_PATH_H_

#include <string>

#include <boost/filesystem.hpp>

namespace ssf {

class Path {
 public:
  Path();
  Path(const Path& path);
  Path(const boost::filesystem::path& path);
  Path(const std::string& path);
  Path(const char* path);

  Path& operator=(const Path& other);
  Path& operator=(const std::string& other);
  Path& operator=(const char* other);

  Path& Path::operator/=(const Path& other);
  Path& Path::operator/=(const std::string& other);
  Path& Path::operator/=(const char* other);

  Path& Path::operator+=(const Path& other);
  Path& Path::operator+=(const std::string& other);
  Path& Path::operator+=(const char* other);

  bool operator==(const Path& path);

  Path GetParent() const;
  Path GetFilename() const;
  Path GetExtension() const;
  bool IsEmpty() const;
  bool IsRelative() const;
  bool IsAbsolute() const;
  bool HasExtension() const;

  Path MakeRelative(const Path& base, boost::system::error_code& ec) const;

  std::string GetString() const;

 private:
  boost::filesystem::path path_;
};

}  // ssf

#endif  // SSF_COMMON_FILESYSTEM_PATH_H_
