#ifndef SSF_LAYER_PROXY_HTTP_CONNECT_REQUEST_H_
#define SSF_LAYER_PROXY_HTTP_CONNECT_REQUEST_H_

#include <map>
#include <string>

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

class HttpConnectRequest {
 private:
  using HeadersMap = std::map<std::string, std::string>;

 public:
  HttpConnectRequest(const std::string& target_host,
                     const std::string& target_port);

  void AddHeader(const std::string& name, const std::string& value);

  std::string GenerateRequest() const;

 private:
  std::string target_host_;
  std::string target_port_;
  HeadersMap headers_;
};

}  // detail
}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_HTTP_CONNECT_REQUEST_H_