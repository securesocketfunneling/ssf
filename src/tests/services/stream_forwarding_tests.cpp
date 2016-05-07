#include <array>
#include <functional>
#include <future>
#include <memory>
#include <vector>

#include <gtest/gtest.h>
#include <boost/asio.hpp>

#include <ssf/log/log.h>

#include "services/initialisation.h"
#include "services/user_services/port_forwarding.h"

#include "tests/services/service_test_fixture.h"
#include "tests/services/tcp_helpers.h"

class StreamForwardTest
    : public ServiceFixtureTest<ssf::services::PortForwading> {
 public:
  StreamForwardTest() {}

  ~StreamForwardTest() {}

  virtual void SetUp() {
    StartServer("127.0.0.1", "8000");
    StartClient("127.0.0.1", "8000");
  }

  std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) {
    return ServiceTested::CreateServiceOptions("5454:127.0.0.1:5354", ec);
  }
};

TEST_F(StreamForwardTest, transferOnesOverStream) {
  ASSERT_TRUE(Wait());

  std::list<std::promise<bool>> clients_finish;

  boost::recursive_mutex mutex;

  auto download = [&mutex](size_t size, std::promise<bool>& test_client) {
    tests::tcp::DummyClient client("127.0.0.1", "5454", size);

    {
      boost::recursive_mutex::scoped_lock lock(mutex);
      EXPECT_TRUE(client.Run());
    }

    client.Stop();
    test_client.set_value(true);
  };

  tests::tcp::DummyServer serv("127.0.0.1", "5354");
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