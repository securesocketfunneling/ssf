#include <sstream>

#include "ssf/layer/proxy/http_request.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

HttpRequest::HttpRequest()
    : method_(), uri_(), headers_() {
}

void HttpRequest::Reset(const std::string& method, const std::string& uri) {
  method_ = method;
  uri_ = uri;
  headers_.clear();
  body_.clear();
  AddHeader("Connection", "keep-alive");
}

void HttpRequest::AddHeader(const std::string& name, const std::string& value) {
  headers_[name] = value;
}

std::string HttpRequest::Header(const std::string& name) {
  HeadersMap::const_iterator it = headers_.find(name);
  if (it == headers_.end()) {
    // header not found
    return "";
  }

  return it->second;
}

std::string HttpRequest::GenerateRequest() const {
  std::stringstream ss_request;
  std::string eol("\r\n");

  ss_request << "CONNECT " << uri_ << " HTTP/1.1" << eol;

  for (const auto& header : headers_) {
    ss_request << header.first << ": " << header.second << eol;
  }
  ss_request << body_;
  ss_request << eol;

  return ss_request.str();
}

}  // detail
}  // proxy
}  // layer
}  // ssf