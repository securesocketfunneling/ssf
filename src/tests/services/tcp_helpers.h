#ifndef TESTS_SERVICES_TCP_HELPERS_H_
#define TESTS_SERVICES_TCP_HELPERS_H_

#include <memory>
#include <set>
#include <thread>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/read.hpp>

#include <ssf/log/log.h>

namespace tests {
namespace tcp {

class DummyServer {
 public:
  DummyServer(const std::string& listening_addr,
              const std::string& listening_port);

  void Run();

  void Stop();

 private:
  void DoAccept();

  void HandleAccept(std::shared_ptr<boost::asio::ip::tcp::socket> p_socket,
                    const boost::system::error_code& ec);

  void DoSendOnes(std::shared_ptr<boost::asio::ip::tcp::socket> p_socket,
                  std::shared_ptr<std::size_t> p_size,
                  const boost::system::error_code& ec, size_t length);

  void HandleSend(std::shared_ptr<boost::asio::ip::tcp::socket> p_socket,
                  std::shared_ptr<std::size_t> p_size,
                  const boost::system::error_code& ec, size_t length);

  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_worker_;
  boost::asio::ip::tcp::acceptor acceptor_;
  std::string listening_addr_;
  std::string listening_port_;
  size_t one_buffer_size_;
  std::array<uint8_t, 10240> one_buffer_;
  std::set<std::shared_ptr<boost::asio::ip::tcp::socket>> sockets_;
  std::vector<std::thread> threads_;
};

class DummyClient {
 public:
  DummyClient(const std::string& target_addr, const std::string& target_port,
              size_t size);

  bool Run();

  void Stop();

 private:
  bool CheckOneBuffer(size_t n);

  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> p_worker_;
  boost::asio::ip::tcp::socket socket_;
  std::thread t_;
  std::string target_addr_;
  std::string target_port_;
  size_t size_;
  std::array<uint8_t, 10240> one_buffer_;
};

}  // tcp
}  // tests

#endif  // TESTS_SERVICES_TCP_HELPERS_H_