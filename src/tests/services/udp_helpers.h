#ifndef TESTS_SERVICES_UDP_HELPERS_H_
#define TESTS_SERVICES_UDP_HELPERS_H_

#include <memory>
#include <set>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/read.hpp>
#include <boost/thread.hpp>

#include <ssf/log/log.h>

namespace tests {
namespace udp {

class DummyServer {
 public:
  DummyServer(const std::string& listening_addr,
              const std::string& listening_port);

  void Run();

  void Stop();

 private:
  void DoReceive();

  void SizeReceivedHandler(
      std::shared_ptr<boost::asio::ip::udp::endpoint> p_endpoint,
      std::shared_ptr<size_t> p_size, const boost::system::error_code& ec,
      size_t length);

  void OneBufferSentHandler(
      std::shared_ptr<boost::asio::ip::udp::endpoint> p_endpoint,
      std::shared_ptr<size_t> p_size, const boost::system::error_code& ec,
      size_t length);

  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_worker_;
  std::string listening_addr_;
  std::string listening_port_;
  boost::asio::ip::udp::socket socket_;
  boost::recursive_mutex one_buffer_mutex_;
  std::vector<uint8_t> one_buffer_;
  boost::thread_group threads_;
};

class DummyClient {
 public:
  DummyClient(const std::string& target_addr, const std::string& target_port,
              size_t size);

  bool Init();

  bool ReceiveOneBuffer();

  void Stop();

 private:
  void ResetBuffer();

  bool CheckOneBuffer(size_t n);

 private:
  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_worker_;
  boost::asio::ip::udp::socket socket_;
  boost::asio::ip::udp::endpoint endpoint_;
  boost::thread t_;
  std::string target_addr_;
  std::string target_port_;
  size_t size_;
  std::array<uint8_t, 10240> one_buffer_;
};

}  // udp
}  // tests

#endif  // TESTS_SERVICES_UDP_HELPERS_H_