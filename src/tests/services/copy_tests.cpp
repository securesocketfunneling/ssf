#include "tests/services/copy_fixture_test.h"

TEST_F(CopyFixtureTest, CopyNoFileFromClientToServerTest) {
  std::string server_port("6050");
  StartServer(server_port);
  StartClient(server_port);

  ASSERT_TRUE(Wait());

  auto input_file = GetInputDirectory();
  input_file /= "test_filex.txt";
  auto output_file = GetInputDirectory();
  output_file /= "test_filex.txt";
  ssf::services::copy::CopyRequest req(false, false, false, true, 5,
                                       input_file.GetString(),
                                       output_file.GetString());
  StartCopy(req, true);

  ASSERT_TRUE(WaitClose());

  // Check that file does not exists
  ASSERT_FALSE(boost::filesystem::is_regular_file(output_file.GetString()))
      << "The file " << output_file.GetString() << " should not exist in "
      << GetInputDirectory().GetString();
}

TEST_F(CopyFixtureTest, InvalidInputDirectoryFromClientToServerTest) {
  std::string server_port("6050");
  StartServer(server_port);
  StartClient(server_port);

  ASSERT_TRUE(Wait());

  ssf::Path input_file("unknown/input/directory");
  input_file /= "test_filex.txt";
  auto output_file = GetOutputDirectory();
  output_file /= "test_filex.txt";
  ssf::services::copy::CopyRequest req(false, false, false, true, 1,
                                       input_file.GetString(),
                                       output_file.GetString());
  StartCopy(req, true);

  ASSERT_TRUE(WaitClose());

  // Check that file does not exists
  ASSERT_FALSE(boost::filesystem::is_regular_file(output_file.GetString()))
      << "The file " << output_file.GetString() << " should not exist in "
      << GetInputDirectory().GetString();
}

TEST_F(CopyFixtureTest, InvalidInputDirectoryFromServerToClientTest) {
  std::string server_port("6050");
  StartServer(server_port);
  StartClient(server_port);

  ASSERT_TRUE(Wait());

  ssf::Path input_file("unknown/input/directory");
  input_file /= "test_filex.txt";
  auto output_file = GetOutputDirectory();
  output_file /= "test_filex.txt";
  ssf::services::copy::CopyRequest req(false, false, false, true, 1,
                                       input_file.GetString(),
                                       output_file.GetString());
  StartCopy(req, false);

  ASSERT_TRUE(WaitClose());

  // Check that file does not exists
  ASSERT_FALSE(boost::filesystem::is_regular_file(output_file.GetString()))
      << "The file " << output_file.GetString() << " should not exist in "
      << GetInputDirectory().GetString();
}

TEST_F(CopyFixtureTest, InvalidOutputDirectoryFromClientToServerTest) {
  std::string server_port("6050");
  StartServer(server_port);
  StartClient(server_port);

  ASSERT_TRUE(Wait());

  ssf::Path input_file(GenerateRandomFile(
      GetInputDirectory().GetString(), "test_file", ".txt", 1024 * 1024 * 10));

  ssf::Path output_file("unknown/output/directory");
  output_file /= "test_filex.txt";
  ssf::services::copy::CopyRequest req(false, false, false, true, 1,
                                       input_file.GetString(),
                                       output_file.GetString());
  StartCopy(req, true);

  ASSERT_TRUE(WaitClose());

  // Check that file does not exists
  ASSERT_FALSE(boost::filesystem::is_regular_file(output_file.GetString()))
      << "The file " << output_file.GetString() << " should not exist in "
      << GetInputDirectory().GetString();
}

TEST_F(CopyFixtureTest, CopyUniqueFileFromClientToServerTest) {
  std::string server_port("6100");

  StartServer(server_port);
  StartClient(server_port);

  ASSERT_TRUE(Wait());

  ssf::Path file(GenerateRandomFile(GetInputDirectory().GetString(),
                                    "test_file", ".txt", 1024 * 1024 * 10));

  auto output_file = GetOutputDirectory();
  output_file /= "output_file.txt";

  ssf::services::copy::CopyRequest req(
      false, false, false, true, 5, file.GetString(), output_file.GetString());
  StartCopy(req, true);

  ASSERT_TRUE(WaitClose());

  ASSERT_TRUE(AreFilesEqual(file.GetString(), output_file.GetString()));
}

TEST_F(CopyFixtureTest, CopyGlobFileFromClientToServerTest) {
  std::string server_port("6200");
  StartServer(server_port);
  StartClient(server_port);

  std::list<ssf::Path> files;
  for (uint32_t i = 0; i <= 10; ++i) {
    files.emplace_back(GenerateRandomFile(GetInputDirectory(), "test_file",
                                          ".txt", 1024 * 1024 * i));
  }

  ASSERT_TRUE(Wait());

  ssf::Path input_pattern(GetInputDirectory());
  input_pattern /= "*.txt";

  ssf::services::copy::CopyRequest req(false, false, false, true, 5,
                                       input_pattern.GetString(),
                                       GetOutputDirectory().GetString());
  StartCopy(req, true);

  ASSERT_TRUE(WaitClose());

  for (const auto& file : files) {
    ssf::Path output_path(GetOutputDirectory());
    output_path /= file.GetFilename();
    ASSERT_TRUE(AreFilesEqual(file.GetString(), output_path.GetString()));
  }
}

TEST_F(CopyFixtureTest, CopyGlobFileFromClientToServerSequentiallyTest) {
  std::string server_port("6200");
  StartServer(server_port);
  StartClient(server_port);

  std::list<ssf::Path> files;
  for (uint32_t i = 0; i <= 10; ++i) {
    files.emplace_back(GenerateRandomFile(GetInputDirectory(), "test_file",
                                          ".txt", 1024 * 1024 * i));
  }

  ASSERT_TRUE(Wait());

  ssf::Path input_pattern(GetInputDirectory());
  input_pattern /= "*.txt";

  ssf::services::copy::CopyRequest req(false, false, false, true, 5,
                                       input_pattern.GetString(),
                                       GetOutputDirectory().GetString());
  StartCopy(req, true);

  ASSERT_TRUE(WaitClose());

  for (const auto& file : files) {
    ssf::Path output_path(GetOutputDirectory());
    output_path /= file.GetFilename();
    ASSERT_TRUE(AreFilesEqual(file.GetString(), output_path.GetString()));
  }
}

TEST_F(CopyFixtureTest, CopyUniqueFileFromServerToClientTest) {
  std::string server_port("6300");
  StartServer(server_port);
  StartClient(server_port);

  ASSERT_TRUE(Wait());

  ssf::Path file(GenerateRandomFile(GetInputDirectory(), "test_file", ".txt",
                                    1024 * 1024 * 10));

  auto output_file = GetOutputDirectory();
  output_file /= "output_file.txt";

  ssf::services::copy::CopyRequest req(
      false, false, false, true, 1, file.GetString(), output_file.GetString());
  StartCopy(req, false);

  ASSERT_TRUE(WaitClose());

  ASSERT_TRUE(AreFilesEqual(file.GetString(), output_file.GetString()));
}

TEST_F(CopyFixtureTest, CopyGlobFileFromServerToClientTest) {
  std::string server_port("6400");
  StartServer(server_port);
  StartClient(server_port);

  ASSERT_TRUE(Wait());

  std::list<ssf::Path> files;
  for (uint32_t i = 0; i <= 10; ++i) {
    files.emplace_back(GenerateRandomFile(GetInputDirectory(), "test_file",
                                          ".txt", 1024 * 1024 * i));
  }

  ssf::Path input_pattern(GetInputDirectory());
  input_pattern /= "*.txt";

  ssf::services::copy::CopyRequest req(false, false, false, true, 5,
                                       input_pattern.GetString(),
                                       GetOutputDirectory().GetString());
  StartCopy(req, false);

  ASSERT_TRUE(WaitClose());

  for (const auto& file : files) {
    ssf::Path output_path(GetOutputDirectory());
    output_path /= file.GetFilename();
    ASSERT_TRUE(AreFilesEqual(file.GetString(), output_path.GetString()));
  }
}

TEST_F(CopyFixtureTest, CopyGlobFileFromServerToClientSequentiallyTest) {
  std::string server_port("6400");
  StartServer(server_port);
  StartClient(server_port);

  ASSERT_TRUE(Wait());

  std::list<ssf::Path> files;
  for (uint32_t i = 0; i <= 10; ++i) {
    files.emplace_back(GenerateRandomFile(GetInputDirectory(), "test_file",
                                          ".txt", 1024 * 1024 * i));
  }

  ssf::Path input_pattern(GetInputDirectory());
  input_pattern /= "*.txt";

  ssf::services::copy::CopyRequest req(false, false, false, true, 1,
                                       input_pattern.GetString(),
                                       GetOutputDirectory().GetString());
  StartCopy(req, false);

  ASSERT_TRUE(WaitClose());

  for (const auto& file : files) {
    ssf::Path output_path(GetOutputDirectory());
    output_path /= file.GetFilename();
    ASSERT_TRUE(AreFilesEqual(file.GetString(), output_path.GetString()));
  }
}

TEST_F(CopyFixtureTest, CopyStdinFromClientToServerTest) {
  std::string server_port("6500");
  StartServer(server_port);
  StartClient(server_port);

  ssf::Path file(GenerateRandomFile(GetInputDirectory(), "test_file", ".txt",
                                    1024 * 1024 * 10));

  // stdin as generated file
  std::ifstream in(file.GetString(), std::ifstream::binary);
  std::streambuf* cinbuf = std::cin.rdbuf();
  std::cin.rdbuf(in.rdbuf());

  ASSERT_TRUE(Wait());

  ssf::Path output_file(GetOutputDirectory());
  output_file /= "output_file.txt";

  ssf::services::copy::CopyRequest req(true, false, false, false, 1, "",
                                       output_file.GetString());
  StartCopy(req, true);

  ASSERT_TRUE(WaitClose());

  ASSERT_TRUE(AreFilesEqual(file.GetString(), output_file.GetString()));

  // restore stdin
  std::cin.rdbuf(cinbuf);
}

TEST_F(CopyFixtureTest, CopyEmptyStdinFromClientToServerTest) {
  std::string server_port("6500");
  StartServer(server_port);
  StartClient(server_port);

  ASSERT_TRUE(Wait());

  // close stdin
  fclose(stdin);

  ssf::Path output_file(GetOutputDirectory());
  output_file /= "output_file.txt";

  ssf::services::copy::CopyRequest req(true, false, false, false, 1, "",
                                       output_file.GetString());
  StartCopy(req, true);

  ASSERT_TRUE(WaitClose());

  // Check that file does not exists
  ASSERT_TRUE(boost::filesystem::is_regular_file(output_file.GetString()))
      << "File " << output_file.GetString() << " should exist in "
      << GetInputDirectory().GetString();
  ASSERT_EQ(boost::filesystem::file_size(output_file.GetString()), 0)
      << "File " << output_file.GetString() << " should be empty";
}

TEST_F(CopyFixtureTest, ResumeCopyFromClientToServerTest) {
  std::string server_port("6300");
  StartServer(server_port);
  StartClient(server_port);

  ASSERT_TRUE(Wait());

  const std::size_t kMaxFileSize = 1024 * 1024 * 10;
  ssf::Path input_file(GenerateRandomFile(GetInputDirectory(), "test_file",
                                          ".txt", kMaxFileSize));

  auto output_file = GetOutputDirectory();
  output_file /= "output_file.txt";

  std::array<char, 4096> buffer;
  uint64_t total_read = 0;
  std::ifstream input_fh(input_file.GetString(), std::ifstream::binary);
  std::ofstream output_fh(output_file.GetString(),
                          std::ofstream::binary | std::ofstream::trunc);
  while (total_read < (kMaxFileSize / 4) && input_fh.good() &&
         output_fh.good()) {
    input_fh.read(buffer.data(), buffer.size());
    output_fh.write(buffer.data(), input_fh.gcount());
    total_read += input_fh.gcount();
  }
  input_fh.close();
  output_fh.close();

  ssf::services::copy::CopyRequest req(false, true, false, true, 5,
                                       input_file.GetString(),
                                       output_file.GetString());
  StartCopy(req, false);

  ASSERT_TRUE(WaitClose());

  ASSERT_TRUE(AreFilesEqual(input_file.GetString(), output_file.GetString()));
}

TEST_F(CopyFixtureTest, ResumeCopyFromServerToClientTest) {
  std::string server_port("6300");
  StartServer(server_port);
  StartClient(server_port);

  ASSERT_TRUE(Wait());

  const std::size_t kMaxFileSize = 1024 * 1024 * 10;
  ssf::Path input_file(GenerateRandomFile(GetInputDirectory(), "test_file",
                                          ".txt", kMaxFileSize));

  auto output_file = GetOutputDirectory();
  output_file /= "output_file.txt";

  std::array<char, 4096> buffer;
  uint64_t total_read = 0;
  std::ifstream input_fh(input_file.GetString(), std::ifstream::binary);
  std::ofstream output_fh(output_file.GetString(),
                          std::ofstream::binary | std::ofstream::trunc);
  while (total_read < (kMaxFileSize / 4) && input_fh.good() &&
         output_fh.good()) {
    input_fh.read(buffer.data(), buffer.size());
    output_fh.write(buffer.data(), input_fh.gcount());
    total_read += input_fh.gcount();
  }
  input_fh.close();
  output_fh.close();

  ssf::services::copy::CopyRequest req(false, true, false, true, 5,
                                       input_file.GetString(),
                                       output_file.GetString());
  StartCopy(req, true);

  ASSERT_TRUE(WaitClose());

  ASSERT_TRUE(AreFilesEqual(input_file.GetString(), output_file.GetString()));
}

TEST_F(CopyFixtureTest, InvalidResumeCopyFromClientToServerTest) {
  std::string server_port("6300");
  StartServer(server_port);
  StartClient(server_port);

  ASSERT_TRUE(Wait());

  const std::size_t kMaxFileSize = 1024 * 1024 * 10;
  ssf::Path input_file(GenerateRandomFile(GetInputDirectory(), "test_file",
                                          ".txt", kMaxFileSize));

  ssf::Path output_file(GenerateRandomFile(GetOutputDirectory(), "test_file",
                                           ".txt", kMaxFileSize / 2));

  ssf::services::copy::CopyRequest req(false, true, false, true, 5,
                                       input_file.GetString(),
                                       output_file.GetString());
  StartCopy(req, false);

  ASSERT_TRUE(WaitClose());

  ASSERT_FALSE(AreFilesEqual(input_file, output_file));
}

TEST_F(CopyFixtureTest, InvalidResumeCopyFromServerToClientTest) {
  std::string server_port("6300");
  StartServer(server_port);
  StartClient(server_port);

  ASSERT_TRUE(Wait());

  const std::size_t kMaxFileSize = 1024 * 1024 * 10;
  ssf::Path input_file(GenerateRandomFile(GetInputDirectory(), "test_file",
                                          ".txt", kMaxFileSize));

  ssf::Path output_file(GenerateRandomFile(GetOutputDirectory(), "test_file",
                                           ".txt", kMaxFileSize / 2));

  ssf::services::copy::CopyRequest req(false, true, false, true, 5,
                                       input_file.GetString(),
                                       output_file.GetString());
  StartCopy(req, true);

  ASSERT_TRUE(WaitClose());

  ASSERT_FALSE(AreFilesEqual(input_file, output_file));
}