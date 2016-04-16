#include "tests/copy_file_test_fixture.h"

#include <algorithm>
#include <iostream>

#include <openssl/md5.h>

using Md5Digest = std::array<unsigned char, MD5_DIGEST_LENGTH>;

Md5Digest GetMd5Sum(const std::string& filepath,
                    boost::system::error_code& ec) {
  using Buffer = std::array<char, 1024>;

  Md5Digest md5 = {{0}};
  Buffer buf = {{0}};

  std::ifstream file(filepath, std::ifstream::binary);
  MD5_CTX md5_context;

  if (!file.is_open()) {
    ec.assign(boost::system::errc::bad_file_descriptor,
              boost::system::get_system_category());
    return md5;
  }

  MD5_Init(&md5_context);
  do {
    file.read(buf.data(), buf.size());
    MD5_Update(&md5_context, buf.data(),
               static_cast<std::size_t>(file.gcount()));
  } while (!file.eof());

  MD5_Final(md5.data(), &md5_context);

  return md5;
}

void FileExistsAndIdentical(const std::string& source_filepath,
                            const std::string& test_filepath) {
  ASSERT_TRUE(boost::filesystem::is_regular_file(test_filepath))
      << "The file " << test_filepath << " should exist in files_copied";

  // Test source_file is equal to test_file
  // This test could fail due to the closing of fiber to file session after the
  // execution of this test
  boost::system::error_code ec;
  Md5Digest source_digest = GetMd5Sum(source_filepath, ec);
  ASSERT_EQ(ec.value(), 0) << "Source file could not be digested";
  Md5Digest test_digest = GetMd5Sum(test_filepath, ec);
  ASSERT_EQ(ec.value(), 0) << "Test file could not be digested";

  ASSERT_TRUE(std::equal(source_digest.begin(), source_digest.end(),
                         test_digest.begin()))
      << "MD5 sum of file " << source_filepath << " and " << test_filepath
      << " are different";
}

//-----------------------------------------------------------------------------
TEST_F(CopyNoFileFromClientToRemoteTest, CopyTest) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::info);

  ASSERT_TRUE(Wait());

  ASSERT_TRUE(WaitClose());

  // Check if file exists and content is equal
  boost::filesystem::path copied_file("files_copied/test_filex.txt");
  ASSERT_FALSE(boost::filesystem::is_regular_file(copied_file))
      << "The file test_filex.txt should not exist in files_copied";
}

//-----------------------------------------------------------------------------
TEST_F(CopyUniqueFileFromClientToRemoteTest, CopyTest) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::info);

  ASSERT_TRUE(Wait());

  ASSERT_TRUE(WaitClose());

  FileExistsAndIdentical("files_to_copy/test_file1.txt",
                         "files_copied/test_file1.txt");
}

//-----------------------------------------------------------------------------
TEST_F(CopyGlobFileFromClientToRemoteTest, CopyTest) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::info);

  ASSERT_TRUE(Wait());

  ASSERT_TRUE(WaitClose());

  FileExistsAndIdentical("files_to_copy/test_file1.txt",
                         "files_copied/test_file1.txt");
  FileExistsAndIdentical("files_to_copy/test_file2.txt",
                         "files_copied/test_file2.txt");
}

//-----------------------------------------------------------------------------
TEST_F(CopyUniqueFileFromRemoteToClientTest, CopyTest) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::info);

  ASSERT_TRUE(Wait());

  ASSERT_TRUE(WaitClose());

  FileExistsAndIdentical("files_to_copy/test_file1.txt",
                         "files_copied/test_file1.txt");
}

//-----------------------------------------------------------------------------
TEST_F(CopyGlobFileFromRemoteToClientTest, CopyTest) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::info);

  ASSERT_TRUE(Wait());

  ASSERT_TRUE(WaitClose());

  FileExistsAndIdentical("files_to_copy/test_file1.txt",
                         "files_copied/test_file1.txt");
  FileExistsAndIdentical("files_to_copy/test_file2.txt",
                         "files_copied/test_file2.txt");
}

//-----------------------------------------------------------------------------
TEST_F(CopyStdinFromClientToRemoteTest, CopyTest) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::info);

  // stdin as test_file1.txt filebuf
  std::ifstream in("files_to_copy/test_file1.txt", std::ifstream::binary);
  std::streambuf* cinbuf = std::cin.rdbuf();
  std::cin.rdbuf(in.rdbuf());

  ASSERT_TRUE(Wait());

  ASSERT_TRUE(WaitClose());

  FileExistsAndIdentical("files_to_copy/test_file1.txt",
                         "files_copied/test_file1.txt");

  // restore stdin
  std::cin.rdbuf(cinbuf);
}