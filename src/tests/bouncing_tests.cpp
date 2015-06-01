#include <array>
#include <functional>
#include <future>
#include <memory>
#include <vector>

#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "common/config/config.h"

#include "core/client/client.h"
#include "core/server/server.h"

#include "core/network_virtual_layer_policies/link_policies/ssl_policy.h"
#include "core/network_virtual_layer_policies/link_policies/tcp_policy.h"
#include "core/network_virtual_layer_policies/link_authentication_policies/null_link_authentication_policy.h"
#include "core/network_virtual_layer_policies/bounce_protocol_policy.h"
#include "core/transport_virtual_layer_policies/transport_protocol_policy.h"

#include "services/initialisation.h"
#include "services/user_services/port_forwarding.h"

class DummyClient {
 public:
  DummyClient(size_t size)
      : io_service_(),
        p_worker_(new boost::asio::io_service::work(io_service_)),
        socket_(io_service_),
        size_(size) {}

  bool Run() {
    t_ = boost::thread([&]() { io_service_.run(); });

    boost::asio::ip::tcp::resolver r(io_service_);
    boost::asio::ip::tcp::resolver::query q("127.0.0.1", "5454");
    boost::system::error_code ec;
    boost::asio::connect(socket_, r.resolve(q), ec);

    if (ec) {
      BOOST_LOG_TRIVIAL(error) << "dummy client : fail to connect "
                               << ec.value();
      return false;
    }

    boost::asio::write(socket_, boost::asio::buffer(&size_, sizeof(size_t)),
                       ec);

    if (ec) {
      BOOST_LOG_TRIVIAL(error) << "dummy client : fail to write " << ec.value();
      return false;
    }

    size_t received(0);
    size_t n(0);
    while (received < size_) {
      boost::system::error_code ec_read;
      n = socket_.read_some(boost::asio::buffer(one_buffer_), ec_read);

      if (n == 0) {
        return false;
      }

      if (ec_read) {
        return false;
      }

      if (!CheckOneBuffer(n)) {
        return false;
      }
      received += n;
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
  bool CheckOneBuffer(size_t n) {
    for (size_t i = 0; i < n; ++i) {
      if (one_buffer_[i] != 1) {
        return false;
      }
    }

    return true;
  }

  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_worker_;
  boost::asio::ip::tcp::socket socket_;
  boost::thread t_;
  size_t size_;
  std::array<uint8_t, 10240> one_buffer_;
};

class DummyServer {
 public:
  DummyServer()
      : io_service_(),
        p_worker_(new boost::asio::io_service::work(io_service_)),
        acceptor_(io_service_),
        one_buffer_size_(10240),
        one_buffer_(one_buffer_size_) {
    for (size_t i = 0; i < one_buffer_size_; ++i) {
      one_buffer_[i] = 1;
    }
  }

  void Run() {
    for (uint8_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
      threads_.create_thread([&]() { io_service_.run(); });
    }

    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), 5354);
    boost::asio::socket_base::reuse_address option(true);

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(option);
    acceptor_.bind(endpoint);
    acceptor_.listen();

    do_accept();
  }

  void Stop() {
    boost::system::error_code ec;
    acceptor_.close(ec);

    p_worker_.reset();

    threads_.join_all();
    io_service_.stop();
  }

 private:
  void do_accept() {
    auto p_socket = std::make_shared<boost::asio::ip::tcp::socket>(io_service_);
    acceptor_.async_accept(*p_socket, boost::bind(&DummyServer::handle_accept,
                                                  this, p_socket, _1));
  }

  void handle_accept(std::shared_ptr<boost::asio::ip::tcp::socket> p_socket,
                     const boost::system::error_code& ec) {
    if (!ec) {
      auto p_size = std::make_shared<size_t>(0);
      boost::asio::async_read(*p_socket,
                              boost::asio::buffer(p_size.get(), sizeof(size_t)),
                              boost::bind(&DummyServer::handle_send, this,
                                          p_socket, p_size, true, _1, _2));
      do_accept();
    } else {
      p_socket->close();
    }
  }

  void handle_send(std::shared_ptr<boost::asio::ip::tcp::socket> p_socket,
                   std::shared_ptr<size_t> p_size, bool first,
                   const boost::system::error_code& ec,
                   size_t tranferred_bytes) {
    if (!ec) {
      if (!first) {
        (*p_size) -= tranferred_bytes;
      }

      if ((*p_size)) {
        if ((*p_size) > one_buffer_size_) {
          boost::asio::async_write(
              *p_socket, boost::asio::buffer(one_buffer_, one_buffer_size_),
              boost::bind(&DummyServer::handle_send, this, p_socket, p_size,
                          false, _1, _2));
        } else {
          boost::asio::async_write(
              *p_socket, boost::asio::buffer(one_buffer_, (*p_size)),
              boost::bind(&DummyServer::handle_send, this, p_socket, p_size,
                          false, _1, _2));
        }
      } else {
        boost::asio::async_read(*p_socket, boost::asio::buffer(one_buffer_, 1),
                                [=](const boost::system::error_code&,
                                    size_t) { p_socket->close(); });
      }
    } else {
      p_socket->close();
    }
  }

  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_worker_;
  boost::asio::ip::tcp::acceptor acceptor_;
  size_t one_buffer_size_;
  std::vector<uint8_t> one_buffer_;
  boost::thread_group threads_;
};

std::string get_remote_addr(const std::string& remote_endpoint_string) {
  size_t position = remote_endpoint_string.find(":");

  if (position != std::string::npos) {
    return remote_endpoint_string.substr(0, position);
  } else {
    return "";
  }
}

std::string get_remote_port(const std::string& remote_endpoint_string) {
  size_t position = remote_endpoint_string.find(":");

  if (position != std::string::npos) {
    return remote_endpoint_string.substr(position + 1);
  } else {
    return "";
  }
}

//-----------------------------------------------------------------------------
TEST(BouncingTests, BouncingChain) {
  using Server =
      ssf::SSFServer<ssf::SSLPolicy, ssf::NullLinkAuthenticationPolicy,
                     ssf::BounceProtocolPolicy, ssf::TransportProtocolPolicy>;
  using Client =
      ssf::SSFClient<ssf::SSLPolicy, ssf::NullLinkAuthenticationPolicy,
                     ssf::BounceProtocolPolicy, ssf::TransportProtocolPolicy>;

  typedef boost::asio::ip::tcp::socket socket;
  typedef ssf::SSLWrapper<> ssl_socket;
  typedef boost::asio::fiber::basic_fiber_demux<ssl_socket> Demux;
  typedef ssf::services::BaseUserService<Demux>::BaseUserServicePtr
    BaseUserServicePtr;

  boost::log::core::get()->set_filter(boost::log::trivial::severity >=
                                      boost::log::trivial::info);

  boost::recursive_mutex mutex;
  std::promise<bool> network_set;
  std::promise<bool> transport_set;
  std::promise<bool> service_set;

  boost::asio::io_service client_io_service;
  std::unique_ptr<boost::asio::io_service::work> p_client_worker(
    new boost::asio::io_service::work(client_io_service));
  boost::thread_group client_threads;
  for (uint8_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    client_threads.create_thread([&]() { client_io_service.run(); });
  }

  boost::asio::io_service bouncer_io_service;
  std::unique_ptr<boost::asio::io_service::work> p_bouncer_worker(
    new boost::asio::io_service::work(bouncer_io_service));
  boost::thread_group bouncer_threads;
  for (uint8_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    bouncer_threads.create_thread([&]() { bouncer_io_service.run(); });
  }

  boost::asio::io_service server_io_service;
  std::unique_ptr<boost::asio::io_service::work> p_server_worker(
    new boost::asio::io_service::work(server_io_service));
  boost::thread_group server_threads;
  for (uint8_t i = 1; i <= boost::thread::hardware_concurrency(); ++i) {
    server_threads.create_thread([&]() { server_io_service.run(); });
  }

  std::list<Server> servers;
  std::list<std::string> bouncers;

  uint16_t initial_server_port = 10000;
  uint8_t nb_of_servers = 10;

  {
    ssf::Config ssf_config;
    ++initial_server_port;
    servers.emplace_front(server_io_service, ssf_config, initial_server_port);
    servers.front().run();
    bouncers.emplace_front(std::string("127.0.0.1:") +
      std::to_string(initial_server_port));
  }

    for (uint8_t i = 0; i < nb_of_servers - 1; ++i) {
      ssf::Config ssf_config;
      ++initial_server_port;
      servers.emplace_front(bouncer_io_service, ssf_config, initial_server_port);
      servers.front().run();
      bouncers.emplace_front(std::string("127.0.0.1:") +
                             std::to_string(initial_server_port));
  }

    std::vector<BaseUserServicePtr> client_options;
    std::string error_msg;
    //auto p_service = ssf::services::PortForwading<Demux>::CreateServiceOptions(
    //  "5454:127.0.0.1:5354", &error_msg);

    //client_options.push_back(p_service);

    std::map<std::string, std::string> params;

    auto first = bouncers.front();
    bouncers.pop_front();
    params["remote_addr"] = get_remote_addr(first);
    params["remote_port"] = get_remote_port(first);

    std::ostringstream ostrs;
    boost::archive::text_oarchive ar(ostrs);
    ar << BOOST_SERIALIZATION_NVP(bouncers);

    params["bouncing_nodes"] = ostrs.str();

    ssf::Config ssf_config;

    auto callback = [&mutex, &network_set, &service_set, &transport_set](
        ssf::services::initialisation::type type,
        BaseUserServicePtr p_user_service,
        const boost::system::error_code& ec) {
      boost::recursive_mutex::scoped_lock lock(mutex);
      if (type == ssf::services::initialisation::NETWORK) {
        network_set.set_value(!ec);
        if (ec) {
          service_set.set_value(false);
          transport_set.set_value(false);
        }

        return;
      }

      if (type == ssf::services::initialisation::TRANSPORT) {
        transport_set.set_value(!ec);

        return;
      }

      /*if (type == ssf::services::initialisation::SERVICE &&
          p_user_service->GetName() == "forward") {
        service_set.set_value(!ec);

        return;
      }*/
    };

    Client client(client_io_service, "127.0.0.1",
                  std::to_string(initial_server_port - nb_of_servers),
                  ssf_config, client_options, std::move(callback));
    client.run(params);

    network_set.get_future().wait();
    transport_set.get_future().wait();
    //service_set.get_future().wait();

    { boost::recursive_mutex::scoped_lock lock(mutex); }

    client.stop();
    for (auto& server : servers) {
      server.stop();
    }


    p_client_worker.reset();
    client_threads.join_all();
    client_io_service.stop();

    p_bouncer_worker.reset();
    bouncer_threads.join_all();
    bouncer_io_service.stop();

    p_server_worker.reset();
    server_threads.join_all();
    server_io_service.stop();

    servers.clear();
}