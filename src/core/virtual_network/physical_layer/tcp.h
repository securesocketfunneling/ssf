#ifndef SSF_CORE_VIRTUAL_NETWORK_PHYSICAL_LAYER_TCP_H_
#define SSF_CORE_VIRTUAL_NETWORK_PHYSICAL_LAYER_TCP_H_

#include <cstdint>

#include <memory>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/system/error_code.hpp>

#include "core/virtual_network/basic_empty_stream.h"
#include "core/virtual_network/parameters.h"
#include "core/virtual_network/physical_layer/tcp_helpers.h"
#include "core/virtual_network/protocol_attributes.h"


namespace virtual_network {
namespace physical_layer {

class tcp {
 public:
  enum {
    id = 1,
    overhead = 0,
    facilities = virtual_network::facilities::stream,
    mtu = 65535 - overhead
  };
  enum { endpoint_stack_size = 1 };

  static const char* NAME;

  typedef int socket_context;
  typedef int acceptor_context;
  typedef boost::asio::ip::tcp::socket socket;
  typedef boost::asio::ip::tcp::acceptor acceptor;
  typedef boost::asio::ip::tcp::resolver resolver;
  typedef boost::asio::ip::tcp::endpoint endpoint;

 private:
  using query = ParameterStack;
  using ptree = boost::property_tree::ptree;

 public:
  operator boost::asio::ip::tcp() { return boost::asio::ip::tcp::v4(); }

  static std::string get_name() {
    return NAME;
  }

  static endpoint make_endpoint(boost::asio::io_service& io_service,
                                query::const_iterator parameters_it, uint32_t,
                                boost::system::error_code& ec) {
    return virtual_network::physical_layer::detail::make_tcp_endpoint(
        io_service, *parameters_it, ec);
  }

  static void add_params_from_property_tree(
      query* p_query, const boost::property_tree::ptree& property_tree,
      bool connect, boost::system::error_code& ec) {
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

    virtual_network::ptree_entry_to_query(*layer_parameters, "port", &params);
    virtual_network::ptree_entry_to_query(*layer_parameters, "addr", &params);

    p_query->push_back(params);
  }
};

const char* tcp::NAME = "TCP";

using TCPPhysicalLayer = VirtualEmptyStreamProtocol<tcp>;

}  // physical_layer
}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_PHYSICAL_LAYER_TCP_H_
