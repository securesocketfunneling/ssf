#include <algorithm>

#include "ssf/layer/proxy/http_response.h"

namespace ssf {
namespace layer {
namespace proxy {

HttpResponse::HttpResponse() : status_code_(0), headers_() {}

bool HttpResponse::Success() const { return status_code_ == StatusCode::kOk; }

bool HttpResponse::CloseConnection() const {
  return HeaderContains("connection", "close") != std::string::npos;
}

bool HttpResponse::Redirected() const {
  return (status_code_ == StatusCode::kMovedTemporarily ||
          status_code_ == StatusCode::kMovedPermanently) &&
         (headers_.find("location") != headers_.end());
}

bool HttpResponse::AuthenticationRequired() const {
  return (status_code_ == StatusCode::kUnauthorized ||
          status_code_ == StatusCode::kProxyAuthenticationRequired) &&
         ((headers_.find("proxy-authenticate") != headers_.end()) ||
          (headers_.find("www-authenticate") != headers_.end()));
}

bool HttpResponse::IsAuthenticationAllowed(const std::string& auth_name) const {
  return HasHeaderValueBeginsWith("proxy-authenticate", auth_name) ||
         HasHeaderValueBeginsWith("www-authenticate", auth_name);
}

std::size_t HttpResponse::HeaderContains(const std::string& header_name,
                                         const std::string& content) const {
  HeaderValues values = GetHeaderValues(header_name);
  if (values.empty()) {
    return false;
  }

  for (const auto& value : values) {
    auto find_pos = value.find(content);
    if (find_pos != std::string::npos) {
      return find_pos;
    }
  }

  return std::string::npos;
}

bool HttpResponse::HasHeaderValueBeginsWith(
    const std::string& header_name, const std::string& begin_with) const {
  return HeaderContains(header_name, begin_with) == 0;
}

void HttpResponse::AddHeader(const std::string& name,
                             const std::string& value) {
  std::string name_lower = name;
  std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                 ::tolower);
  HeadersMap::const_iterator it = headers_.find(name_lower);
  if (it == headers_.end()) {
    headers_[name_lower] = {};
  }

  headers_[name_lower].push_back(value);
}

void HttpResponse::Reset() {
  status_code_ = 0;
  headers_.clear();
  body_.clear();
}

HttpResponse::HeaderValues HttpResponse::GetHeaderValues(const std::string& name) const {
  std::string name_lower = name;
  std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                 ::tolower);
  HeadersMap::const_iterator it = headers_.find(name_lower);
  if (it == headers_.end()) {
    // header not found
    return {};
  }

  return it->second;
}

}  // proxy
}  // layer
}  // ssf