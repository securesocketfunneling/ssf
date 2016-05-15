#include <vector>
#include <functional>
#include <array>
#include <future>
#include <list>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <gtest/gtest.h>

#include <ssf/log/log.h>

#include "services/initialisation.h"
#include "services/user_services/remote_process.h"

#include "tests/services/service_test_fixture.h"

class RemoteProcessTest
    : public ServiceFixtureTest<ssf::services::RemoteProcess> {
 public:
  RemoteProcessTest() {}

  ~RemoteProcessTest() {}

  virtual void SetUp() {
    StartServer("127.0.0.1", "9000");
    StartClient("127.0.0.1", "9000");
  }

  virtual std::shared_ptr<ServiceTested> ServiceCreateServiceOptions(
      boost::system::error_code& ec) {
    return ServiceTested::CreateServiceOptions("9091", ec);
  }
};

TEST_F(RemoteProcessTest, StartProcessTest) {
  ASSERT_TRUE(Wait());

  boost::asio::io_service io_service;
  boost::thread t([&]() { io_service.run(); });

  boost::asio::ip::tcp::socket socket(io_service);

  boost::asio::ip::tcp::resolver r(io_service);
  boost::asio::ip::tcp::resolver::query q("127.0.0.1", "9091");
  boost::system::error_code ec;

  boost::asio::connect(socket, r.resolve(q), ec);

  if (ec) {
    SSF_LOG(kLogError) << "client: fail to connect " << ec.value();
  }

  socket.close();
}
