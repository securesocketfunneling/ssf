#include <algorithm>
#include <sstream>
#include <string>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <boost/algorithm/string.hpp>

#include "ssf/layer/proxy/basic_auth_strategy.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

BasicAuthStrategy::BasicAuthStrategy()
    : AuthStrategy(Status::kAuthenticating), request_populated_(false) {}

bool BasicAuthStrategy::Support(const HttpResponse& response) const {
  return !request_populated_ &&
         (response.HeaderValueBeginWith("Proxy-Authenticate", "Basic") ||
          response.HeaderValueBeginWith("WWW-Authenticate", "Basic"));
}

void BasicAuthStrategy::ProcessResponse(const HttpResponse& response) {
  if (response.Success()) {
    status_ = Status::kAuthenticated;
    return;
  }

  if (!Support(response)) {
    status_ = Status::kAuthenticationFailure;
    return;
  }

  set_proxy_authentication(response);
}

void BasicAuthStrategy::PopulateRequest(
    const ProxyEndpointContext& proxy_ep_ctx, HttpRequest* p_request) {
  const auto& http_proxy = proxy_ep_ctx.http_proxy;

  std::stringstream ss_credentials, header_value;
  ss_credentials << http_proxy.username << ":" << http_proxy.password;

  header_value << "Basic " << Base64Encode(ss_credentials.str());

  p_request->AddHeader(
      proxy_authentication() ? "Proxy-Authorization" : "Authorization",
      header_value.str());

  request_populated_ = true;
}

std::string BasicAuthStrategy::Base64Encode(const std::string& input) {
  using namespace boost::archive::iterators;
  using Base64Iterator =
      base64_from_binary<transform_width<std::string::const_iterator, 6, 8> >;

  std::stringstream ss_encoded_input;

  std::copy(Base64Iterator(input.begin()), Base64Iterator(input.end()),
            ostream_iterator<char>(ss_encoded_input));

  // padding with '='
  switch (input.size() % 3) {
    case 1:
      ss_encoded_input << "==";
      break;
    case 2:
      ss_encoded_input << '=';
      break;
    default:
      break;
  }

  return ss_encoded_input.str();
}

}  // detail
}  // proxy
}  // layer
}  // ssf