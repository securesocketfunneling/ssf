#ifndef TESTS_SERVICES_COPY_FIXTURE_TEST_H_
#define TESTS_SERVICES_COPY_FIXTURE_TEST_H_

#include <algorithm>
#include <array>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <vector>

#include <openssl/md5.h>

#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

#include "common/config/config.h"

#include "core/network_protocol.h"

#include "core/client/client.h"
#include "core/server/server.h"

#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/copy/copy_client.h"
#include "services/copy/packet/control.h"
#include "services/user_services/copy.h"
#include "services/user_services/parameters.h"

using NetworkProtocol = ssf::network::NetworkProtocol;

class CopyFixtureTest : public ::testing::Test {
 public:
  using Client = ssf::Client;
  using ClientSessionPtr = ssf::Client::ClientSessionPtr;
  using Server =
      ssf::SSFServer<NetworkProtocol::Protocol, ssf::TransportProtocolPolicy>;
  using Demux = Client::Demux;
  using UserServicePtr =
      ssf::services::BaseUserService<Demux>::BaseUserServicePtr;
  using Copy = ssf::services::Copy<Demux>;
  using CopyClient = ssf::services::copy::CopyClient;
  using CopyClientPtr = ssf::services::copy::CopyClientPtr;
  using Md5Digest = std::array<unsigned char, MD5_DIGEST_LENGTH>;

 protected:
  CopyFixtureTest();

  static bool AreFilesEqual(const ssf::Path& source_filepath,
                            const ssf::Path& test_filepath);

  static ssf::Path GetInputDirectory();
  static ssf::Path GetOutputDirectory();

  ssf::Path GenerateRandomFile(const ssf::Path& file_dir,
                               const std::string& name_prefix,
                               const std::string& file_suffix,
                               uint64_t filesize);

  void StartClient(const std::string& server_port);
  void StartServer(const std::string& server_port);
  bool Wait();
  bool WaitClose();
  void StopServerThreads();
  void StopClientThreads();
  void OnClientStatus(ssf::Status status);
  void OnClientUserServiceStatus(UserServicePtr p_user_service,
                                 const boost::system::error_code& ec);

  bool StartCopy(const ssf::services::copy::CopyRequest& req,
                 bool from_client_to_server);

  void SetUp() override;
  void TearDown() override;

 protected:
  std::unique_ptr<Client> p_ssf_client_;
  std::unique_ptr<Server> p_ssf_server_;
  CopyClientPtr copy_client_;

  std::promise<bool> network_set_;
  std::promise<bool> transport_set_;
  std::promise<bool> service_set_;
  std::promise<bool> close_set_;
};

#endif  // TESTS_SERVICES_COPY_FIXTURE_TEST_H_
