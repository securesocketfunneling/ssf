#include "common/filesystem/path.h"

namespace ssf {

Path::Path() : path_() {}

Path::Path(const Path& path) : path_() { path_ = path.GetString(); }

Path::Path(const boost::filesystem::path& path) : path_(path) {}

Path::Path(const char* path) : path_() { path_ = path; }

Path::Path(const std::string& path) : path_() { path_ = path; }

Path& Path::operator=(const Path& other) {
  path_ = other.GetString();
  return *this;
}

Path& Path::operator=(const std::string& other) {
  path_ = other;
  return *this;
}

Path& Path::operator=(const char* other) {
  path_ = other;
  return *this;
}

Path& Path::operator+=(const Path& other) {
  path_ += other.path_;
  return *this;
}

Path& Path::operator+=(const std::string& other) {
  path_ += boost::filesystem::path(other);
  return *this;
}

Path& Path::operator+=(const char* other) {
  path_ += boost::filesystem::path(other);
  return *this;
}

Path& Path::operator/=(const Path& other) {
  path_ /= other.path_;
  return *this;
}

Path& Path::operator/=(const std::string& other) {
  path_ /= boost::filesystem::path(other);
  return *this;
}

Path& Path::operator/=(const char* other) {
  path_ /= boost::filesystem::path(other);
  return *this;
}

bool Path::operator==(const Path& path) {
  return GetString() == path.GetString();
}

Path Path::GetParent() const { return Path(path_.parent_path()); }

Path Path::GetFilename() const { return Path(path_.filename()); }

Path Path::GetExtension() const { return Path(path_.extension()); }

bool Path::IsEmpty() const { return path_.empty(); }

bool Path::IsRelative() const { return path_.is_relative(); }

bool Path::IsAbsolute() const { return path_.is_absolute(); }

bool Path::HasExtension() const { return path_.has_extension(); }

Path Path::MakeRelative(const Path& base, boost::system::error_code& ec) const {
  return Path(boost::filesystem::relative(path_, base.path_, ec));
}

std::string Path::GetString() const { return path_.generic_string(); }

}  // ssf