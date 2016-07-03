#ifndef SSF_LAYER_PROXY_HTTP_REQUEST_H_
#define SSF_LAYER_PROXY_HTTP_REQUEST_H_

#include <map>
#include <string>

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

class HttpRequest {
 private:
  using HeadersMap = std::map<std::string, std::string>;

 public:
  HttpRequest(const std::string& method, const std::string& uri);

  inline std::string method() const { return method_; }
  inline std::string uri() const { return uri_; }
  inline std::string body() const { return body_; }
  inline void set_body(const std::string& body) { body_ = body; }

  void AddHeader(const std::string& name, const std::string& value);

  std::string Header(const std::string& name);

  std::string GenerateRequest() const;

 private:
  std::string method_;
  std::string uri_;
  HeadersMap headers_;
  std::string body_;
};

}  // detail
}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_HTTP_REQUEST_H_