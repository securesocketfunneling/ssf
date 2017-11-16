#include "common/filesystem/filesystem.h"

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
  std::list<Path> result;

  ssf::Path input_dir(filepath_pattern);
  if (!IsDirectory(input_dir, ec)) {
    input_dir = filepath_pattern.GetParent();
  }
  ec.clear();

  // convert pattern to regex
  std::regex filepath_regex;
  try {
    std::regex special_chars_regex("[.^$\\[\\]{}+\\\\]");
    std::string filepath_filter = std::regex_replace(
      filepath_pattern.GetString(), special_chars_regex, "[&]",
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

  ListFilesRec(input_dir, Path(""), recursive, filepath_regex, result,  ec);

  return result;
}

void Filesystem::ListFilesRec(const Path& input_dir,
                              const Path& base_dir,
                              bool recursive,
                              const std::regex& filepath_regex,
                              std::list<Path>& files,
                              boost::system::error_code& ec) const {
  Path cur_dir = input_dir / base_dir;

  boost::filesystem::directory_iterator directory_it(cur_dir.GetString(), ec);
  if (ec)
    return;

  while (directory_it != boost::filesystem::directory_iterator()) {
    Path p = directory_it->path();

    if (IsFile(p, ec) && std::regex_search(p.GetString(), filepath_regex)) {
      files.emplace_back(base_dir / p.GetFilename());
    } else if (recursive && IsDirectory(directory_it->path(), ec)) {
      ListFilesRec(input_dir, base_dir / p.GetFilename(), recursive, filepath_regex, files, ec);
    }

    ++directory_it;
  }
}

}  // ssf
