#ifndef SSF_LAYER_PHYSICAL_TCP_H_
#define SSF_LAYER_PHYSICAL_TCP_H_

#include <cstdint>

#include <memory>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

#include "ssf/layer/basic_empty_stream.h"
#include "ssf/layer/parameters.h"
#include "ssf/layer/physical/tcp_helpers.h"
#include "ssf/layer/protocol_attributes.h"
#include "ssf/layer/physical/host.h"

namespace ssf {
namespace layer {
namespace physical {

class tcp {
 public:
  enum {
    id = 1,
    overhead = 0,
    facilities = ssf::layer::facilities::stream,
    mtu = 65535 - overhead
  };
  enum { endpoint_stack_size = 1 };

  static const char* NAME;

  using socket_context = int;
  using acceptor_context = int;
  using acceptor = boost::asio::ip::tcp::acceptor;
  using endpoint = boost::asio::ip::tcp::endpoint;
  using resolver = boost::asio::ip::tcp::resolver;
  using socket = boost::asio::ip::tcp::socket;

 private:
  using query = ParameterStack;
  using ptree = boost::property_tree::ptree;

 public:
  operator boost::asio::ip::tcp() { return boost::asio::ip::tcp::v4(); }

  static std::string get_name() { return NAME; }

  static endpoint make_endpoint(boost::asio::io_service& io_service,
                                query::const_iterator parameters_it, uint32_t,
                                boost::system::error_code& ec) {
    return ssf::layer::physical::detail::make_tcp_endpoint(io_service,
                                                           *parameters_it, ec);
  }

  static std::string get_address(const endpoint& endpoint) {
    return endpoint.address().to_string();
  }

  static unsigned short get_port(const endpoint& endpoint) {
    return endpoint.port();
  }

  static void add_params_from_property_tree(query* p_query,
                                            const ptree& property_tree,
                                            bool connect,
                                            boost::system::error_code& ec) {
    auto layer_name = property_tree.get_child_optional("layer");
    if (!layer_name || layer_name.get().data() != NAME) {
      ec.assign(ssf::error::invalid_argument, ssf::error::get_ssf_category());
      return;
    }

    LayerParameters params;
    auto layer_parameters = property_tree.get_child_optional("parameters");
    if (!layer_parameters) {
      ec.assign(ssf::error::missing_config_parameters,
                ssf::error::get_ssf_category());
      return;
    }

    ssf::layer::ptree_entry_to_query(*layer_parameters, "port", &params);
    ssf::layer::ptree_entry_to_query(*layer_parameters, "addr", &params);

    p_query->push_back(params);
  }
};

using TCPPhysicalLayer = VirtualEmptyStreamProtocol<tcp>;

}  // physical
}  // layer
}  // ssf

#endif  // SSF_LAYER_PHYSICAL_TCP_H_
