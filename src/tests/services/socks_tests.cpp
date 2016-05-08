#include <vector>
#include <functional>
#include <array>
#include <future>
#include <list>

#include <gtest/gtest.h>
#include <boost/asio.hpp>

#include <ssf/log/log.h>

#include "services/initialisation.h"
#include "services/user_services/socks.h"

#include "tests/services/service_test_fixture.h"
#include "tests/services/socks_helpers.h"
#include "tests/services/tcp_helpers.h"

class SocksTest : public ServiceFixtureTest<ssf::services::Socks> {
 public:
  SocksTest() {}

  ~SocksTest() {}

  virtual void SetUp() {
    StartServer("127.0.0.1", "9000");
    StartClient("127.0.0.1", "9000");
  }

  virtual std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) {
    return ServiceTested::CreateServiceOptions("9091", ec);
  }
};

TEST_F(SocksTest, startStopTransmitSSFSocks) {
  ASSERT_TRUE(Wait());

  std::list<std::promise<bool>> clients_finish;

  boost::recursive_mutex mutex;

  auto download = [&mutex](size_t size, std::promise<bool>& test_client) {
    tests::socks::DummyClient client("127.0.0.1", "9091", "127.0.0.1", "9090",
                                     size);
    auto initiated = client.Init();

    {
      boost::recursive_mutex::scoped_lock lock(mutex);
      EXPECT_TRUE(initiated);
    }

    auto sent = client.InitSocks();

    {
      boost::recursive_mutex::scoped_lock lock(mutex);
      EXPECT_TRUE(sent);
    }

    auto received = client.ReceiveOneBuffer();

    {
      boost::recursive_mutex::scoped_lock lock(mutex);
      EXPECT_TRUE(received);
    }

    client.Stop();
    test_client.set_value(true);
  };

  tests::tcp::DummyServer serv("127.0.0.1", "9090");
  serv.Run();

  boost::thread_group client_test_threads;

  for (std::size_t i = 0; i < 6; ++i) {
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
