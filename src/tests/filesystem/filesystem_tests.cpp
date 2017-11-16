#include <fstream>
#include <string>
#include <regex>

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

#include "common/filesystem/filesystem.h"

static constexpr char kFilesystemDirectory[] = "filesystem";

class FilesystemTest : public ::testing::Test {
 protected:
  void SetUp() {
    if (boost::filesystem::is_directory(kFilesystemDirectory)) {
      boost::system::error_code ec;
      for (boost::filesystem::directory_iterator end_it,
           it(kFilesystemDirectory);
           it != end_it; ++it) {
        boost::filesystem::remove_all(it->path(), ec);
      }
    }
  }

  ssf::Path GenerateRandomFile(const ssf::Path& file_dir,
                               const std::string& name_prefix,
                               const std::string& file_suffix,
                               uint64_t filesize) {
    auto file_path = file_dir;
    file_path /= name_prefix;
    file_path += std::to_string(rand());
    file_path += file_suffix;

    std::ofstream file(file_path.GetString());

    for (uint64_t i = 0; i < filesize / sizeof(int); ++i) {
      int random = rand();
      file.write(reinterpret_cast<const char*>(&random), sizeof(int));
    }

    return file_path;
  }

 protected:
  ssf::Filesystem fs_;
};

TEST_F(FilesystemTest, BasicTests) {
  boost::system::error_code ec;

  ssf::Path directory_base(kFilesystemDirectory);
  ssf::Path directory_path(directory_base);
  ssf::Path base_pattern(directory_base);
  ssf::Path txt_pattern(directory_base);
  ssf::Path unknown_pattern(directory_base);

  ASSERT_TRUE(fs_.IsDirectory(directory_path, ec));
  ASSERT_EQ(ec.value(), 0);

  directory_path /= "test1/test2/test3";
  ASSERT_TRUE(fs_.MakeDirectories(directory_path, ec));
  ASSERT_EQ(ec.value(), 0);
  ASSERT_TRUE(fs_.IsDirectory(directory_path, ec));
  ASSERT_EQ(ec.value(), 0);

  for (uint32_t i = 0; i < 5; ++i) {
    GenerateRandomFile(directory_base, "file", ".base", i * 1024 * 1024);
  }
  for (uint32_t i = 0; i < 10; ++i) {
    GenerateRandomFile(directory_path, "file", ".txt", i * 1024 * 1024);
  }

  auto files = fs_.ListFiles(directory_base, false, ec);
  ASSERT_EQ(ec.value(), 0);
  ASSERT_EQ(files.size(), 5);

  files = fs_.ListFiles(directory_base, true, ec);
  ASSERT_EQ(ec.value(), 0);
  ASSERT_EQ(files.size(), 15);

  base_pattern /= "*.base";
  files = fs_.ListFiles(base_pattern, true, ec);
  ASSERT_EQ(ec.value(), 0);
  ASSERT_EQ(files.size(), 5);

  txt_pattern /= "*.txt";
  files = fs_.ListFiles(txt_pattern, true, ec);
  ASSERT_EQ(ec.value(), 0);
  ASSERT_EQ(files.size(), 10);

  unknown_pattern /= "*.cpp";
  files = fs_.ListFiles(unknown_pattern, true, ec);
  ASSERT_EQ(ec.value(), 0);
  ASSERT_EQ(files.size(), 0);
}
