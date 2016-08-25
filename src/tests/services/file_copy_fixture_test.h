#ifndef TESTS_SERVICES_COPY_FILE_FIXTURE_TEST_H_
#define TESTS_SERVICES_COPY_FILE_FIXTURE_TEST_H_

#include <array>
#include <functional>
#include <future>
#include <memory>
#include <vector>

#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include "common/config/config.h"

#include "core/network_protocol.h"

#include "core/client/client.h"
#include "core/server/server.h"

#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/initialisation.h"
#include "services/user_services/copy_file_service.h"

using NetworkProtocol = ssf::network::NetworkProtocol;

class FileCopyTestFixture : public ::testing::Test {
 public:
  using Client =
      ssf::SSFClient<NetworkProtocol::Protocol, ssf::TransportProtocolPolicy>;
  using Server =
      ssf::SSFServer<NetworkProtocol::Protocol, ssf::TransportProtocolPolicy>;
  using demux = Client::Demux;
  using BaseUserServicePtr =
      ssf::services::BaseUserService<demux>::BaseUserServicePtr;

 protected:
  FileCopyTestFixture();

  virtual ~FileCopyTestFixture();

  void StartServer();
  bool Wait();
  bool WaitClose();
  void StopServerThreads();
  void StopClientThreads();
  void SSFClientCallback(ssf::services::initialisation::type type,
                         BaseUserServicePtr p_user_service,
                         const boost::system::error_code& ec);

  void SetUp() override;
  void StartClient();
  void TearDown() override;

  virtual std::string GetOutputPattern() const;
  virtual bool GetFromStdin() const = 0;
  virtual bool GetFromLocalToRemote() const = 0;
  virtual std::string GetInputPattern() const = 0;

 protected:
  std::unique_ptr<Client> p_ssf_client_;
  std::unique_ptr<Server> p_ssf_server_;

  std::promise<bool> network_set_;
  std::promise<bool> transport_set_;
  std::promise<bool> service_set_;
  std::promise<bool> close_set_;
};

class CopyNoFileFromClientToRemoteTest : public FileCopyTestFixture {
 public:
  CopyNoFileFromClientToRemoteTest() : FileCopyTestFixture() {}

  bool GetFromStdin() const override { return false; }

  bool GetFromLocalToRemote() const override { return true; }

  std::string GetInputPattern() const override {
    return "files_to_copy/test_filex.txt";
  }
};

class CopyUniqueFileFromClientToRemoteTest : public FileCopyTestFixture {
 public:
  CopyUniqueFileFromClientToRemoteTest() : FileCopyTestFixture() {}

  bool GetFromStdin() const override { return false; }

  bool GetFromLocalToRemote() const override { return true; }

  std::string GetInputPattern() const override {
    return "files_to_copy/test_file1.txt";
  }
};

class CopyGlobFileFromClientToRemoteTest : public FileCopyTestFixture {
 public:
  CopyGlobFileFromClientToRemoteTest() : FileCopyTestFixture() {}

  bool GetFromStdin() const override { return false; }

  bool GetFromLocalToRemote() const override { return true; }

  std::string GetInputPattern() const override {
    return "files_to_copy/test_*.txt";
  }
};

class CopyUniqueFileFromRemoteToClientTest : public FileCopyTestFixture {
 public:
  CopyUniqueFileFromRemoteToClientTest() : FileCopyTestFixture() {}

  bool GetFromStdin() const override { return false; }

  bool GetFromLocalToRemote() const override { return false; }

  std::string GetInputPattern() const override {
    return "files_to_copy/test_file1.txt";
  }
};

class CopyGlobFileFromRemoteToClientTest : public FileCopyTestFixture {
 public:
  CopyGlobFileFromRemoteToClientTest() : FileCopyTestFixture() {}

  bool GetFromStdin() const override { return false; }

  bool GetFromLocalToRemote() const override { return false; }

  std::string GetInputPattern() const override {
    return "files_to_copy/test_*.txt";
  }
};

class CopyStdinFromClientToRemoteTest : public FileCopyTestFixture {
 public:
  CopyStdinFromClientToRemoteTest() : FileCopyTestFixture() {}

  bool GetFromStdin() const override { return true; }

  bool GetFromLocalToRemote() const override { return true; }

  std::string GetInputPattern() const override {
    return "files_to_copy/test_*.txt";
  }

  std::string GetOutputPattern() const override {
    return "files_copied/test_file1.txt";
  }
};

#endif  // TESTS_SERVICES_COPY_FILE_FIXTURE_TEST_H_
