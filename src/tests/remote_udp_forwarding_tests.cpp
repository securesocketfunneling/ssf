#include <vector>
#include <functional>
#include <array>
#include <future>
#include <list>

#include <gtest/gtest.h>
#include <boost/asio.hpp>

#include <ssf/layer/data_link/circuit_helpers.h>
#include <ssf/layer/data_link/basic_circuit_protocol.h>
#include <ssf/layer/data_link/simple_circuit_policy.h>
#include <ssf/layer/parameters.h>
#include <ssf/layer/physical/tcp.h>
#include <ssf/layer/physical/tlsotcp.h>

#include "common/config/config.h"

#include "core/network_protocol.h"
#include "core/client/client.h"
#include "core/server/server.h"

#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/initialisation.h"
#include "services/user_services/udp_remote_port_forwarding.h"

class DummyClient {
 public:
  DummyClient(size_t size)
      : io_service_(),
        p_worker_(new boost::asio::io_service::work(io_service_)),
        socket_(io_service_),
        size_(size) {}

  bool Init() {
    t_ = boost::thread([&]() { io_service_.run(); });

    boost::asio::ip::udp::resolver r(io_service_);
    boost::asio::ip::udp::resolver::query q("127.0.0.1", "5454");

    auto it = r.resolve(q);

    endpoint_ = *it;

    boost::system::error_code ec;
    socket_.open(endpoint_.protocol(), ec);
    socket_.connect(endpoint_, ec);

    if (ec) {
      return false;
    } else {
      return true;
    }
  }

  bool ReceiveOneBuffer() {
    size_t received = 0;

    while (received < size_) {
      boost::system::error_code ec;
      size_t remaining_size = size_ - received;
      socket_.send(boost::asio::buffer(&remaining_size, sizeof(remaining_size)),
                   0, ec);

      if (ec) {
        return false;
      }

      auto n = socket_.receive(boost::asio::buffer(one_buffer_), 0, ec);

      if (ec) {
        return false;
      }

      if (n == 0) {
        return false;
      } else {
        received += n;
        if (!CheckOneBuffer(n)) {
          return false;
        }
        ResetBuffer();
      }
    }
    return received == size_;
  }

  void Stop() {
    boost::system::error_code ec;
    socket_.close(ec);

    p_worker_.reset();

    t_.join();
    io_service_.stop();
  }

 private:
  void ResetBuffer() {
    for (size_t i = 0; i < one_buffer_.size(); ++i) {
      one_buffer_[i] = 0;
    }
  }

  bool CheckOneBuffer(size_t n) {
    for (size_t i = 0; i < n; ++i) {
      if (one_buffer_[i] != 1) {
        return false;
      }
    }

    return true;
  }

 private:
  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_worker_;
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint endpoint_;
  boost::thread t_;
  size_t size_;
  std::array<uint8_t, 10240> one_buffer_;
};

class DummyServer {
 public:
  DummyServer()
      : io_service_(),
        p_worker_(new boost::asio::io_service::work(io_service_)),
        socket_(io_service_),
        one_buffer_(10240) {
    for (size_t i = 0; i < 10240; ++i) {
      one_buffer_[i] = 1;
    }
  }

  void Run() {
    for (uint8_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
      threads_.create_thread([&]() { io_service_.run(); });
    }

    boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::udp::v4(), 5354);

    socket_.open(endpoint.protocol());
    socket_.bind(endpoint);

    DoReceive();
  }

  void Stop() {
    boost::system::error_code ec;
    socket_.close(ec);

    p_worker_.reset();

    threads_.join_all();
    io_service_.stop();
  }

 private:
  void DoReceive() {
    auto p_new_size_ = std::make_shared<size_t>(0);
    auto p_send_endpoint_ = std::make_shared<boost::asio::ip::udp::endpoint>();

    socket_.async_receive_from(
        boost::asio::buffer(&(*p_new_size_), sizeof((*p_new_size_))),
        *p_send_endpoint_, boost::bind(&DummyServer::SizeReceivedHandler, this,
                                       p_send_endpoint_, p_new_size_, _1, _2));
  }

  void SizeReceivedHandler(
      std::shared_ptr<boost::asio::ip::udp::endpoint> p_endpoint,
      std::shared_ptr<size_t> p_size, const boost::system::error_code& ec,
      size_t length) {
    if (!ec) {
      {
        boost::recursive_mutex::scoped_lock lock(one_buffer_mutex_);
        socket_.async_send_to(boost::asio::buffer(one_buffer_, *p_size),
                              *p_endpoint,
                              boost::bind(&DummyServer::OneBufferSentHandler,
                                          this, p_endpoint, p_size, _1, _2));
      }
    }
  }

  void OneBufferSentHandler(
      std::shared_ptr<boost::asio::ip::udp::endpoint> p_endpoint,
      std::shared_ptr<size_t> p_size, const boost::system::error_code& ec,
      size_t length) {
    if (ec.value() == ::error::message_too_long) {
      {
        boost::recursive_mutex::scoped_lock lock(one_buffer_mutex_);
        one_buffer_.resize(one_buffer_.size() / 2);
      }
      if (one_buffer_.size()) {
        SizeReceivedHandler(p_endpoint, p_size, boost::system::error_code(), 0);
        return;
      }
    } else {
      DoReceive();
      return;
    }
  }

  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_worker_;
  boost::asio::ip::udp::socket socket_;
  boost::recursive_mutex one_buffer_mutex_;
  std::vector<uint8_t> one_buffer_;
  boost::thread_group threads_;
};

class RemoteUdpForwardTest : public ::testing::Test {
 public:
  using Client =
      ssf::SSFClient<ssf::network::Protocol, ssf::TransportProtocolPolicy>;
  using Server =
      ssf::SSFServer<ssf::network::Protocol, ssf::TransportProtocolPolicy>;

  using demux = Client::Demux;

  using BaseUserServicePtr =
      ssf::services::BaseUserService<demux>::BaseUserServicePtr;

 public:
  RemoteUdpForwardTest() : p_ssf_client_(nullptr), p_ssf_server_(nullptr) {}

  ~RemoteUdpForwardTest() {}

  virtual void SetUp() {
    StartServer();
    StartClient();
  }

  virtual void TearDown() {
    p_ssf_client_->Stop();
    p_ssf_server_->Stop();
  }

  void StartServer() {
    ssf::Config ssf_config;

    auto endpoint_query =
        ssf::network::GenerateServerQuery("", "8000", ssf_config);

    p_ssf_server_.reset(new Server());

    boost::system::error_code run_ec;
    p_ssf_server_->Run(endpoint_query, run_ec);
  }

  void StartClient() {
    std::vector<BaseUserServicePtr> client_options;
    boost::system::error_code ec;
    auto p_service =
        ssf::services::UdpRemotePortForwading<demux>::CreateServiceOptions(
            "5454:127.0.0.1:5354", ec);

    client_options.push_back(p_service);

    ssf::Config ssf_config;

    auto endpoint_query =
        ssf::network::GenerateClientQuery("127.0.0.1", "8000", ssf_config, {});

    p_ssf_client_.reset(new Client(
        client_options, boost::bind(&RemoteUdpForwardTest::SSFClientCallback,
                                    this, _1, _2, _3)));

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
        p_user_service->GetName() == "udpremote") {
      service_set_.set_value(!ec);

      return;
    }
  }

 protected:
  std::unique_ptr<Client> p_ssf_client_;
  std::unique_ptr<Server> p_ssf_server_;

  std::promise<bool> network_set_;
  std::promise<bool> transport_set_;
  std::promise<bool> service_set_;
};

//-----------------------------------------------------------------------------
TEST_F(RemoteUdpForwardTest, transferOnesOverUdp) {
  ASSERT_TRUE(Wait());

  std::list<std::promise<bool>> clients_finish;

  boost::recursive_mutex mutex;

  auto download = [&mutex](size_t size, std::promise<bool>& test_client) {
    DummyClient client(size);
    auto initiated = client.Init();

    {
      boost::recursive_mutex::scoped_lock lock(mutex);
      EXPECT_TRUE(initiated);
    }

    auto received = client.ReceiveOneBuffer();

    {
      boost::recursive_mutex::scoped_lock lock(mutex);
      EXPECT_TRUE(received);
    }

    client.Stop();
    test_client.set_value(true);
  };

  DummyServer serv;
  serv.Run();

  boost::thread_group client_test_threads;

  for (int i = 0; i < 6; ++i) {
    clients_finish.emplace_front();
    std::promise<bool>& client_finish = clients_finish.front();
    client_test_threads.create_thread(boost::bind<void>(
        download, 1024 * 1024 * i, boost::ref(client_finish)));
  }

  client_test_threads.join_all();
  for (auto& client_finish : clients_finish) {
    client_finish.get_future().wait();
  }

  serv.Stop();
}
