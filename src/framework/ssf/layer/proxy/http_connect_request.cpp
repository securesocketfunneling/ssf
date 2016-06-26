#include <sstream>

#include "ssf/layer/proxy/http_connect_request.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

HttpConnectRequest::HttpConnectRequest(const std::string& target_host,
                                       const std::string& target_port)
    : target_host_(target_host), target_port_(target_port), headers_() {
  headers_["Connection"] = "keep-alive";
}

void HttpConnectRequest::AddHeader(const std::string& name,
                                   const std::string& value) {
  headers_[name] = value;
}

std::string HttpConnectRequest::GenerateRequest() const {
  std::stringstream ss_request;
  std::string eol("\r\n");

  ss_request << "CONNECT " << target_host_ << ":" << target_port_ << " HTTP/1.1"
             << eol;
  ss_request << "Host " << target_host_ << ":" << target_port_ << eol;

  for (const auto& header : headers_) {
    ss_request << header.first << ": " << header.second << eol;
  }

  ss_request << eol;

  return ss_request.str();
}

}  // detail
}  // proxy
}  // layer
}  // ssf