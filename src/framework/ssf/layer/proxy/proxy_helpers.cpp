#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "ssf/error/error.h"
#include "ssf/layer/proxy/proxy_helpers.h"
#include "ssf/log/log.h"
#include "ssf/utils/map_helpers.h"

namespace ssf {
namespace layer {
namespace proxy {

bool ValidateIPTarget(boost::asio::io_service& io_service,
                      const std::string& addr, const std::string& port) {
  boost::system::error_code ec;
  boost::asio::ip::tcp::resolver resolver(io_service);
  boost::asio::ip::tcp::resolver::query query(addr, port);
  resolver.resolve(query, ec);

  return ec.value() == 0;
}

ProxyEndpointContext MakeProxyContext(boost::asio::io_service& io_service,
                                      const LayerParameters& parameters,
                                      boost::system::error_code& ec) {
  ProxyEndpointContext context;
  context.proxy_enabled = false;
  auto http_host = ssf::helpers::GetField<std::string>("http_host", parameters);
  auto http_port = ssf::helpers::GetField<std::string>("http_port", parameters);
  if (http_port.empty() || http_host.empty()) {
    return context;
  }

  if (!ValidateIPTarget(io_service, http_host, http_port)) {
    ec.assign(ssf::error::bad_address, ssf::error::get_ssf_category());
    SSF_LOG(kLogError) << "network[proxy]: could not resolve target address <"
                       << http_host << ":" << http_port << ">";
    return context;
  }

  context.proxy_enabled = true;
  context.http_proxy.host = http_host;
  context.http_proxy.port = http_port;
  context.http_proxy.username =
      ssf::helpers::GetField<std::string>("http_username", parameters);
  context.http_proxy.domain =
      ssf::helpers::GetField<std::string>("http_domain", parameters);
  context.http_proxy.password =
      ssf::helpers::GetField<std::string>("http_password", parameters);
  context.http_proxy.reuse_ntlm =
      (ssf::helpers::GetField<std::string>("http_reuse_ntlm", parameters) ==
       "true");
  context.http_proxy.reuse_kerb =
      (ssf::helpers::GetField<std::string>("http_reuse_kerb", parameters) ==
       "true");
  return context;
}

}  // proxy
}  // layer
}  // ssf