#ifndef TESTS_SERVICES_STREAM_FIXTURE_TEST_H_
#define TESTS_SERVICES_STREAM_FIXTURE_TEST_H_

#include <list>

#include <boost/asio.hpp>

#include "tests/services/service_fixture_test.h"
#include "tests/services/tcp_helpers.h"

template <template <typename> class TServiceTested>
class StreamFixtureTest : public ServiceFixtureTest<TServiceTested> {
 protected:
  void Run(const std::string& in_port, const std::string& server_port) {
    std::list<std::promise<bool>> clients_finish;

    std::recursive_mutex mutex;

    auto download = [&mutex, &in_port, &server_port](
        size_t size, std::promise<bool>& test_client) {
      tests::tcp::DummyClient client("127.0.0.1", in_port, size);

      {
        std::unique_lock<std::recursive_mutex> lock(mutex);
        EXPECT_TRUE(client.Run());
      }

      client.Stop();
      test_client.set_value(true);
    };

    tests::tcp::DummyServer serv("127.0.0.1", server_port);
    serv.Run();

    std::vector<std::thread> client_test_threads;

    for (int i = 0; i < 6; ++i) {
      clients_finish.emplace_front();
      std::promise<bool>& client_finish = clients_finish.front();
      client_test_threads.emplace_back(
          std::bind<void>(download, 1024 * 1024 * i, std::ref(client_finish)));
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

#endif  // TESTS_SERVICES_STREAM_FIXTURE_TEST_H_
