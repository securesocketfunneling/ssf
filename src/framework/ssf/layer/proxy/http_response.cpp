#include <algorithm>

#include "ssf/layer/proxy/http_response.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

HttpResponse::HttpResponse() : status_code_(0), headers_() {}

bool HttpResponse::Success() const { return status_code_ == StatusCode::kOk; }

bool HttpResponse::Redirected() const {
  return (status_code_ == StatusCode::kMovedTemporarily ||
          status_code_ == StatusCode::kMovedPermanently) &&
         (headers_.find("location") != headers_.end());
}

bool HttpResponse::AuthenticationRequired() const {
  return (status_code_ == StatusCode::kUnauthorized ||
          status_code_ == StatusCode::kProxyAuthenticationRequired) &&
         (headers_.find("proxy-authenticate") != headers_.end());
}

bool HttpResponse::HeaderValueBeginWith(const std::string& header_name,
                                        const std::string& begin_with) const {
  auto header = Header(header_name);
  if (header.empty()) {
    return false;
  }

  for (const auto& value : header) {
    if (value.find(begin_with) == 0) {
      return true;
    }
  }

  return false;
}

void HttpResponse::AddHeader(const std::string& name,
                             const std::string& value) {
  auto name_lower = name;
  std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                 ::tolower);
  auto it = headers_.find(name_lower);
  if (it == headers_.end()) {
    headers_[name_lower] = {};
  }

  headers_[name_lower].push_back(value);
}

void HttpResponse::Reset() {
  status_code_ = 0;
  headers_.clear();
}

std::list<std::string> HttpResponse::Header(const std::string& name) const {
  auto name_lower = name;
  std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(),
                 ::tolower);
  HeadersMap::const_iterator it = headers_.find(name_lower);
  if (it == headers_.end()) {
    // header not found
    return {};
  }

  return it->second;
}

}  // detail
}  // proxy
}  // layer
}  // ssf