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

  context.Init(parameters);

  if (context.HttpProxyEnabled() &&
      !ValidateIPTarget(io_service, context.http_proxy().host,
                        context.http_proxy().port)) {
    ec.assign(ssf::error::bad_address, ssf::error::get_ssf_category());
    SSF_LOG(kLogError)
        << "network[http proxy]: could not resolve target address <"
        << context.http_proxy().host << ":" << context.http_proxy().port << ">";
  }
  if (context.SocksProxyEnabled() &&
      !ValidateIPTarget(io_service, context.socks_proxy().host,
                        context.socks_proxy().port)) {
    ec.assign(ssf::error::bad_address, ssf::error::get_ssf_category());
    SSF_LOG(kLogError)
        << "network[socks proxy]: could not resolve target address <"
        << context.socks_proxy().host << ":" << context.socks_proxy().port
        << ">";
  }

  return context;
}

}  // proxy
}  // layer
}  // ssf