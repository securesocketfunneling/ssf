#include "ssf/layer/proxy/connect_op.h"

#include <boost/format.hpp>

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

std::string GenerateHttpProxyRequest(const std::string& target_addr,
                                     const std::string& target_port) {
  boost::format fmt = boost::format(
                          "CONNECT %s:%s HTTP/1.1\r\n"
                          "Host: %s:%s\r\n"
                          "Connection: keep-alive\r\n"
                          "\r\n") %
                      target_addr % target_port % target_addr % target_port;

  return fmt.str();
}

bool CheckHttpProxyResponse(boost::asio::streambuf& streambuf,
                            std::size_t response_size) {
  std::string response_ = std::string(
      boost::asio::buffer_cast<const char*>(streambuf.data()), response_size);

  // Reponse code 200 found ?
  return std::string::npos != response_.find("200");
}

}  // detail
}  // proxy
}  // layer
}  // ssf