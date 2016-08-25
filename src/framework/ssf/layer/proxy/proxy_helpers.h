#ifndef SSF_LAYER_PROXY_PROXY_HELPERS_H_
#define SSF_LAYER_PROXY_PROXY_HELPERS_H_

#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

#include "ssf/layer/parameters.h"

#include "ssf/layer/proxy/proxy_endpoint_context.h"

namespace ssf {
namespace layer {
namespace proxy {

ProxyEndpointContext MakeProxyContext(boost::asio::io_service& io_service,
                                      const LayerParameters& parameters,
                                      boost::system::error_code& ec);

}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_PROXY_HELPERS_H_
