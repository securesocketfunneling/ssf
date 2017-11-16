#ifndef TESTS_SERVICES_SOCKS_FIXTURE_TEST_H_
#define TESTS_SERVICES_SOCKS_FIXTURE_TEST_H_

#include <array>
#include <functional>
#include <list>
#include <thread>
#include <vector>

#include <boost/asio.hpp>

#include "tests/services/service_fixture_test.h"
#include "tests/services/socks_helpers.h"
#include "tests/services/tcp_helpers.h"

template <template <typename> class TServiceTested, class DummyClient>
class SocksFixtureTest : public ServiceFixtureTest<TServiceTested> {
 protected:
  void Run(const std::string& socks_port, const std::string& server_addr,
           const std::string& server_port) {
    std::list<std::promise<bool>> clients_finish;

    auto download = [&socks_port, &server_addr, &server_port](
        size_t size, std::promise<bool>& test_client) {
      DummyClient client("127.0.0.1", socks_port, server_addr, server_port,
                         size);
      auto initiated = client.Init();

      EXPECT_TRUE(initiated);

      auto init = client.InitSocks();

      EXPECT_TRUE(init);

      auto received = client.ReceiveOneBuffer();

      EXPECT_TRUE(received);

      client.Stop();
      test_client.set_value(true);
    };

    tests::tcp::DummyServer serv(server_addr, server_port);
    serv.Run();

    std::vector<std::thread> client_test_threads;

    for (std::size_t i = 0; i <= 5; ++i) {
      clients_finish.emplace_front();
      std::promise<bool>& client_finish = clients_finish.front();
      client_test_threads.emplace_back(std::bind<void>(
          download, 1024 * 1024 * i, std::ref(client_finish)));
    }

    for (auto& thread : client_test_threads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
    for (auto& client_finish : clients_finish) {
      client_finish.get_future().wait();
    }

    serv.Stop();
  }
};

#endif  // TESTS_SERVICES_SOCKS_FIXTURE_TEST_H_