#include "common/filesystem/filesystem.h"

#include <regex>

#include <boost/algorithm/string/replace.hpp>

#include <ssf/log/log.h>

#include "common/error/error.h"

namespace ssf {

Filesystem::Filesystem() {}

bool Filesystem::IsFile(const Path& path, boost::system::error_code& ec) const {
  return boost::filesystem::is_regular_file(path.GetString(), ec);
}

bool Filesystem::IsDirectory(const Path& path,
                             boost::system::error_code& ec) const {
  return boost::filesystem::is_directory(path.GetString(), ec);
}

bool Filesystem::Exists(const Path& path, boost::system::error_code& ec) const {
  auto exists = boost::filesystem::exists(path.GetString(), ec);
  if (ec == boost::system::errc::no_such_file_or_directory) {
    ec.assign(::error::success, ::error::get_ssf_category());
  }
  return exists;
}

uint64_t Filesystem::GetFilesize(const Path& path,
                                 boost::system::error_code& ec) const {
  return boost::filesystem::file_size(path.GetString(), ec);
}

bool Filesystem::MakeDirectory(const Path& path,
                               boost::system::error_code& ec) const {
  return boost::filesystem::create_directory(path.GetString(), ec);
}

bool Filesystem::MakeDirectories(const Path& path,
                                 boost::system::error_code& ec) const {
  return boost::filesystem::create_directories(path.GetString(), ec);
}

bool Filesystem::Remove(const Path& path, boost::system::error_code& ec) const {
  return boost::filesystem::remove(path.GetString(), ec);
}

bool Filesystem::RemoveAll(const Path& path,
                           boost::system::error_code& ec) const {
  return boost::filesystem::remove_all(path.GetString(), ec);
}

std::list<Path> Filesystem::ListFiles(const Path& filepath_pattern, bool recursive,
                                      boost::system::error_code& ec) const {
  ssf::Path input_dir(filepath_pattern);
  if (!IsDirectory(input_dir, ec)) {
    input_dir = filepath_pattern.GetParent();
  }
  ec.clear();
  if (recursive) {
    return ListFilesImpl<boost::filesystem::recursive_directory_iterator>(
        input_dir, filepath_pattern.GetString(), ec);
  } else {
    return ListFilesImpl<boost::filesystem::directory_iterator>(
        input_dir, filepath_pattern.GetString(), ec);
  }
}

template <class DirectoryIterator>
std::list<Path> Filesystem::ListFilesImpl(const Path& input_dir,
                                          const std::string& filepath_pattern,
                                          boost::system::error_code& ec) const {
  std::list<Path> files;

  if (!IsDirectory(input_dir, ec)) {
    return {};
  }

  DirectoryIterator directory_it(input_dir.GetString(), ec);
  if (ec) {
    return {};
  }

  if (directory_it == DirectoryIterator()) {
    return {};
  }

  // convert pattern to regex
  std::regex filepath_regex;
  try {
    std::regex special_chars_regex("[.^$\\[\\]{}+\\\\]");
    std::string filepath_filter = std::regex_replace(
      filepath_pattern, special_chars_regex, "[&]",
      std::regex_constants::match_default | std::regex_constants::format_sed);
    
    boost::replace_all(filepath_filter, "?", "*");
    boost::replace_all(filepath_filter, "*", ".*");

    filepath_regex = std::regex(filepath_filter.c_str(), std::regex::extended |
	                                               std::regex::optimize);
  }
  catch(const std::exception&) {
    SSF_LOG("filesystem", debug, "cannot create filepattern regex");
    return {};
  }

  while (directory_it != DirectoryIterator()) {
    if (IsFile(directory_it->path(), ec)) {
      if (std::regex_search(directory_it->path().generic_string(),
                            filepath_regex)) {
        files.emplace_back(directory_it->path());
      }
    }
    ++directory_it;
  }

  return files;
}

}  // ssf
