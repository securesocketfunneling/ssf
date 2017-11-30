#ifndef TESTS_SERVICES_SHELL_FIXTURE_TEST_H_
#define TESTS_SERVICES_SHELL_FIXTURE_TEST_H_

#include <array>
#include <iostream>
#include <thread>

#include <boost/asio/connect.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <gtest/gtest.h>

#include "tests/services/service_fixture_test.h"

template <template <typename> class TServiceTested>
class ShellFixtureTest : public ServiceFixtureTest<TServiceTested> {
  void SetServerConfig(ssf::config::Config& config) override {
    boost::system::error_code ec;

    const char* new_config = R"RAWSTRING(
{
    "ssf": {
        "services" : {
            "shell": { "enable": true }
        }
    }
}
)RAWSTRING";

    config.UpdateFromString(new_config, ec);
    ASSERT_EQ(ec.value(), 0) << "Could not update server config from string "
                             << new_config;
  }

  void SetClientConfig(ssf::config::Config& config) override {
    boost::system::error_code ec;

    const char* new_config = R"RAWSTRING(
{
    "ssf": {
        "services" : {
            "shell": { "enable": true }
        }
    }
}
)RAWSTRING";

    config.UpdateFromString(new_config, ec);
    ASSERT_EQ(ec.value(), 0) << "Could not update server config from string "
                             << new_config;
  }

 protected:
  void ExecuteCmd(const std::string& shell_port) {
    ASSERT_TRUE(this->Wait());

    boost::asio::io_service io_service;
    std::thread t([&]() { io_service.run(); });
    boost::asio::steady_timer timer(io_service);
    boost::asio::ip::tcp::socket socket(io_service);

    boost::asio::ip::tcp::resolver r(io_service);
    boost::asio::ip::tcp::resolver::query q("127.0.0.1", shell_port);
    boost::system::error_code ec;

    boost::asio::connect(socket, r.resolve(q), ec);

    ASSERT_EQ(ec.value(), 0) << "Fail to connect to client socket";
    std::string command;
#if defined(BOOST_ASIO_WINDOWS)
    command = "dir\n";
#else
    command = "ls -al\n";
#endif

    std::array<char, 512> response_data;

    std::size_t read_bytes = 0, total_read_bytes = 0;

    boost::asio::write(socket, boost::asio::buffer(command), ec);
    ASSERT_EQ(ec.value(), 0) << "Fail to write to socket";

    timer.expires_from_now(std::chrono::seconds(5), ec);
    timer.wait(ec);

    std::cout << std::endl;
    while (socket.available(ec) > 0 && ec.value() == 0) {
      read_bytes = socket.read_some(boost::asio::buffer(response_data), ec);
      total_read_bytes += read_bytes;
      for (uint32_t i = 0; i < read_bytes - 1; ++i) {
        std::cout << response_data[i];
      }
    }
    std::cout << std::endl;

    ASSERT_GT(total_read_bytes, (uint32_t)0) << "No bytes read";
    ASSERT_EQ(ec.value(), (uint32_t)0) << "Fail to read socket";

    socket.close();

    if (t.joinable()) {
      t.join();
    }
  }
};

#endif  // TESTS_SERVICES_SHELL_FIXTURE_TEST_H_
