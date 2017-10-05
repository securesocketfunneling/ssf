#ifndef TESTS_SERVICES_SOCKS_HELPERS_H_
#define TESTS_SERVICES_SOCKS_HELPERS_H_

#include <memory>
#include <thread>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/thread.hpp>

#include <ssf/log/log.h>

namespace tests {
namespace socks {

class SocksDummyClient {
 public:
  bool Init();

  virtual bool InitSocks() = 0;

  bool ReceiveOneBuffer();

  void Stop();

 protected:
  SocksDummyClient(const std::string& socks_server_addr,
                   const std::string& socks_server_port,
                   const std::string& target_addr,
                   const std::string& target_port, size_t size);
  bool CheckOneBuffer(size_t n);

 protected:
  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_worker_;
  boost::asio::ip::tcp::socket socket_;
  std::thread t_;
  std::string socks_server_addr_;
  std::string socks_server_port_;
  std::string target_addr_;
  std::string target_port_;
  std::size_t size_;
  std::array<uint8_t, 10240> one_buffer_;
};

class Socks4DummyClient : public SocksDummyClient {
 public:
  Socks4DummyClient(const std::string& socks_server_addr,
                    const std::string& socks_server_port,
                    const std::string& target_addr,
                    const std::string& target_port, size_t size);

  bool InitSocks();
};

class Socks5DummyClient : public SocksDummyClient {
 public:
  Socks5DummyClient(const std::string& socks_server_addr,
                    const std::string& socks_server_port,
                    const std::string& target_addr,
                    const std::string& target_port, size_t size);

  bool InitSocks();
};

}  // socks
}  // tests

#endif  // TESTS_SERVICES_SOCKS_HELPERS_H_