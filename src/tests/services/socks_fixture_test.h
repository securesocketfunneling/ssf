#ifndef TESTS_SERVICES_SOCKS_FIXTURE_TEST_H_
#define TESTS_SERVICES_SOCKS_FIXTURE_TEST_H_

#include <vector>
#include <functional>
#include <array>
#include <list>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

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

    boost::thread_group client_test_threads;

    for (std::size_t i = 0; i <= 5; ++i) {
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
};

#endif  // TESTS_SERVICES_SOCKS_FIXTURE_TEST_H_