#ifndef SSF_LAYER_PROXY_HTTP_RESPONSE_H_
#define SSF_LAYER_PROXY_HTTP_RESPONSE_H_

#include <list>
#include <map>
#include <string>

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

class HttpResponse {
 private:
  enum StatusCode : int {
    kOk = 200,
    kMovedPermanently = 301,
    kMovedTemporarily = 302,
    kUnauthorized = 401,
    kProxyAuthenticationRequired = 407
  };

  using HeadersMap = std::map<std::string, std::list<std::string>>;

 public:
  HttpResponse();

  inline int status_code() const { return status_code_; }
  inline void set_status_code(int status_code) { status_code_ = status_code; }

  std::list<std::string> Header(const std::string& name) const;
  void AddHeader(const std::string& name, const std::string& value);

  void Reset();

  bool Success() const;
  bool Redirected() const;

  bool HeaderValueBeginWith(const std::string& header_name,
                            const std::string& begin_with) const;

  bool AuthenticationRequired() const;

 private:
  int status_code_;
  HeadersMap headers_;
};

}  // detail
}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_HTTP_RESPONSE_H_
