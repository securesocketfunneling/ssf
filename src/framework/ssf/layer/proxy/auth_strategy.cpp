#include <boost/algorithm/string.hpp>

#include "ssf/layer/proxy/auth_strategy.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

std::string AuthStrategy::ExtractAuthToken(const HttpResponse& response) const {
  std::string challenge_str;
  auto auth_name = AuthName();

  auto proxy_authenticate_hdr = response.Header(
      proxy_authentication() ? "Proxy-Authenticate" : "WWW-Authenticate");
  for (auto& hdr : proxy_authenticate_hdr) {
    // find negotiate auth header
    if (hdr.find(auth_name) == 0) {
      challenge_str = hdr.substr(auth_name.length());
      break;
    }
  }

  if (challenge_str.empty()) {
    return "";
  }

  boost::trim(challenge_str);

  return challenge_str;
}

}  // detail
}  // proxy
}  // layer
}  // ssf