#include <sstream>

#include "ssf/layer/proxy/http_request.h"

namespace ssf {
namespace layer {
namespace proxy {

HttpRequest::HttpRequest() : method_(), uri_(), headers_() {}

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

std::string HttpRequest::GetHeaderValue(const std::string& name) {
  HeadersMap::const_iterator it = headers_.find(name);
  if (it == headers_.end()) {
    // header not found
    return "";
  }

  return it->second;
}

std::string HttpRequest::Serialize() const {
  std::stringstream ss_request;
  std::string eol("\r\n");

  ss_request << method_ << " " << uri_ << " HTTP/1.1" << eol;

  for (const auto& header : headers_) {
    ss_request << header.first << ": " << header.second << eol;
  }
  if (body_.size() > 0) {
    ss_request << "Content-Length: " << body_.size() << eol;
  }
  ss_request << eol;
  ss_request << body_;

  return ss_request.str();
}

}  // proxy
}  // layer
}  // ssf