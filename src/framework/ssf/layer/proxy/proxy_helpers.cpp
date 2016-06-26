#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "ssf/error/error.h"
#include "ssf/layer/proxy/proxy_helpers.h"
#include "ssf/log/log.h"
#include "ssf/utils/map_helpers.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

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
  auto http_addr = ssf::helpers::GetField<std::string>("http_addr", parameters);
  auto http_port = ssf::helpers::GetField<std::string>("http_port", parameters);
  if (http_port == "" || http_addr == "") {
    return context;
  }

  if (!ValidateIPTarget(io_service, http_addr, http_port)) {
    ec.assign(ssf::error::bad_address, ssf::error::get_ssf_category());
    SSF_LOG(kLogError) << "network[proxy]: could not resolve target address <"
                       << http_addr << ":" << http_port << ">";
    return context;
  }

  context.proxy_enabled = true;
  context.http_proxy.addr = http_addr;
  context.http_proxy.port = http_port;
  context.http_proxy.username =
      ssf::helpers::GetField<std::string>("http_username", parameters);
  context.http_proxy.password =
      ssf::helpers::GetField<std::string>("http_password", parameters);
  return context;
}

}  // detail
}  // proxy
}  // layer
}  // ssf