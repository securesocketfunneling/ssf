#include <array>
#include <functional>
#include <future>
#include <memory>
#include <vector>

#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "common/config/config.h"

#include "core/network_protocol.h"

#include "core/client/client.h"
#include "core/server/server.h"

#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/initialisation.h"
#include "services/user_services/copy_file_service.h"

class CopyFileTestFixture : public ::testing::Test {
 public:
  using Client =
      ssf::SSFClient<ssf::network::Protocol, ssf::TransportProtocolPolicy>;
  using Server =
      ssf::SSFServer<ssf::network::Protocol, ssf::TransportProtocolPolicy>;
  using demux = Client::demux;
  using BaseUserServicePtr =
      ssf::services::BaseUserService<demux>::BaseUserServicePtr;

 protected:
  CopyFileTestFixture()
      : client_io_service_(),
        p_client_worker_(new boost::asio::io_service::work(client_io_service_)),
        server_io_service_(),
        p_server_worker_(new boost::asio::io_service::work(server_io_service_)),
        p_ssf_client_(nullptr),
        p_ssf_server_(nullptr) {}

  ~CopyFileTestFixture() {}

  virtual void SetUp() {
    StartServer();
    StartClient();
  }

  virtual void TearDown() {
    StopClientThreads();
    StopServerThreads();

    // Clean receiver path
    boost::filesystem::path receiver_path(GetOutputPattern());
    if (boost::filesystem::is_directory(receiver_path)) {
      for (boost::filesystem::directory_iterator end_it, it(receiver_path);
           it != end_it; ++it) {
        std::remove(it->path().string().c_str());
      }
      return;
    }

    if (boost::filesystem::is_regular_file(receiver_path)) {
      std::remove(GetOutputPattern().c_str());
    }
  }

  void StartServer() {
    ssf::Config ssf_config;

    uint16_t port = 8000;
    auto endpoint_query =
        ssf::network::GenerateServerQuery(std::to_string(port), ssf_config);

    p_ssf_server_.reset(new Server(server_io_service_, ssf_config, 8000));

    StartServerThreads();
    boost::system::error_code run_ec;
    p_ssf_server_->Run(endpoint_query, run_ec);
  }

  virtual void StartClient() {
    std::vector<BaseUserServicePtr> client_services;
    boost::system::error_code ec;
    auto p_service =
        ssf::services::CopyFileService<demux>::CreateServiceFromParams(
            GetFromStdin(), GetFromLocalToRemote(), GetInputPattern(),
            GetOutputPattern(), ec);

    client_services.push_back(p_service);

    ssf::Config ssf_config;

    auto endpoint_query =
        ssf::network::GenerateClientQuery("127.0.0.1", "8000", ssf_config, {});

    p_ssf_client_.reset(new Client(
        client_io_service_, client_services,
        boost::bind(&CopyFileTestFixture::SSFClientCallback, this, _1, _2, _3)));
    StartClientThreads();
    boost::system::error_code run_ec;
    p_ssf_client_->Run(endpoint_query, run_ec);
  }

  bool Wait() {
    auto network_set_future = network_set_.get_future();
    auto service_set_future = service_set_.get_future();
    auto transport_set_future = transport_set_.get_future();

    network_set_future.wait();
    service_set_future.wait();
    transport_set_future.wait();

    return network_set_future.get() && service_set_future.get() &&
           transport_set_future.get();
  }

  bool WaitClose() {
    auto close_future = close_set_.get_future();
    close_future.wait();
    return close_future.get();
  }

  virtual bool GetFromStdin() = 0;
  virtual bool GetFromLocalToRemote() = 0;
  virtual std::string GetInputPattern() = 0;

  virtual std::string GetOutputPattern() { return "files_copied/"; }

  void StartServerThreads() {
    for (uint8_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
      server_threads_.create_thread([&]() { server_io_service_.run(); });
    }
  }

  void StartClientThreads() {
    for (uint8_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
      client_threads_.create_thread([&]() { client_io_service_.run(); });
    }
  }

  void StopServerThreads() {
    p_ssf_server_->Stop();
    p_server_worker_.reset();
    server_threads_.join_all();
    server_io_service_.stop();
  }

  void StopClientThreads() {
    p_client_worker_.reset();
    p_ssf_client_->Stop();
    client_threads_.join_all();
    client_io_service_.stop();
  }

  void SSFClientCallback(ssf::services::initialisation::type type,
                         BaseUserServicePtr p_user_service,
                         const boost::system::error_code& ec) {
    if (type == ssf::services::initialisation::NETWORK) {
      network_set_.set_value(!ec);
      if (ec) {
        service_set_.set_value(false);
        transport_set_.set_value(false);
      }
      return;
    }

    if (type == ssf::services::initialisation::TRANSPORT) {
      transport_set_.set_value(!ec);
      return;
    }

    if (type == ssf::services::initialisation::SERVICE &&
        p_user_service->GetName() == "copy_file") {
      service_set_.set_value(!ec);
      return;
    }

    if (type == ssf::services::initialisation::CLOSE) {
      close_set_.set_value(true);
      return;
    }
  }

 protected:
  boost::asio::io_service client_io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_client_worker_;
  boost::thread_group client_threads_;

  boost::asio::io_service server_io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_server_worker_;
  boost::thread_group server_threads_;
  std::unique_ptr<Client> p_ssf_client_;
  std::unique_ptr<Server> p_ssf_server_;

  std::promise<bool> network_set_;
  std::promise<bool> transport_set_;
  std::promise<bool> service_set_;
  std::promise<bool> close_set_;
};

class CopyNoFileFromClientToRemoteTest : public CopyFileTestFixture {
 public:
  CopyNoFileFromClientToRemoteTest() : CopyFileTestFixture() {}

  virtual bool GetFromStdin() { return false; }

  virtual bool GetFromLocalToRemote() { return true; }

  virtual std::string GetInputPattern() {
    return "files_to_copy/test_filex.txt";
  }
};

class CopyUniqueFileFromClientToRemoteTest : public CopyFileTestFixture {
 public:
  CopyUniqueFileFromClientToRemoteTest() : CopyFileTestFixture() {}

  virtual bool GetFromStdin() { return false; }

  virtual bool GetFromLocalToRemote() { return true; }

  virtual std::string GetInputPattern() {
    return "files_to_copy/test_file1.txt";
  }
};

class CopyGlobFileFromClientToRemoteTest : public CopyFileTestFixture {
 public:
  CopyGlobFileFromClientToRemoteTest() : CopyFileTestFixture() {}

  virtual bool GetFromStdin() { return false; }

  virtual bool GetFromLocalToRemote() { return true; }

  virtual std::string GetInputPattern() { return "files_to_copy/test_*.txt"; }
};

class CopyUniqueFileFromRemoteToClientTest : public CopyFileTestFixture {
 public:
  CopyUniqueFileFromRemoteToClientTest() : CopyFileTestFixture() {}

  virtual bool GetFromStdin() { return false; }

  virtual bool GetFromLocalToRemote() { return false; }

  virtual std::string GetInputPattern() {
    return "files_to_copy/test_file1.txt";
  }
};

class CopyGlobFileFromRemoteToClientTest : public CopyFileTestFixture {
 public:
  CopyGlobFileFromRemoteToClientTest() : CopyFileTestFixture() {}

  virtual bool GetFromStdin() { return false; }

  virtual bool GetFromLocalToRemote() { return false; }

  virtual std::string GetInputPattern() { return "files_to_copy/test_*.txt"; }
};

class CopyStdinFromClientToRemoteTest : public CopyFileTestFixture {
 public:
  CopyStdinFromClientToRemoteTest() : CopyFileTestFixture() {}

  virtual bool GetFromStdin() { return true; }

  virtual bool GetFromLocalToRemote() { return true; }

  virtual std::string GetInputPattern() { return "files_to_copy/test_*.txt"; }

  virtual std::string GetOutputPattern() {
    return "files_copied/test_file1.txt";
  }
};
