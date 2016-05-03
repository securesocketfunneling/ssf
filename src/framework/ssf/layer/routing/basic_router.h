#ifndef SSF_LAYER_ROUTING_BASIC_ROUTER_H_
#define SSF_LAYER_ROUTING_BASIC_ROUTER_H_

#include <cstdint>

#include <boost/asio/async_result.hpp>
#include <boost/asio/basic_io_object.hpp>
#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

#include "ssf/layer/routing/basic_routing_table.h"
#include "ssf/layer/routing/basic_router_service.h"

namespace ssf {
namespace layer {
namespace routing {

template <class NextLayerProtocol,
          class RoutingTable = basic_RoutingTable<NextLayerProtocol>,
          class RouterService =
              basic_Router_service<NextLayerProtocol, RoutingTable>>
class basic_Router : public boost::asio::basic_io_object<RouterService> {
 public:
  typedef typename RouterService::network_address_type network_address_type;
  typedef typename RouterService::prefix_type prefix_type;

  typedef typename RouterService::next_endpoint_type next_endpoint_type;

  using SendDatagram = typename RouterService::SendDatagram;
  using SendQueue = typename RouterService::SendQueue;
  using ReceiveQueue = typename RouterService::ReceiveQueue;

 public:
  basic_Router(boost::asio::io_service& io_service)
      : boost::asio::basic_io_object<RouterService>(io_service) {}

  ~basic_Router() {
    boost::system::error_code ec;
    close(ec);
  }

  boost::system::error_code add_network(prefix_type prefix,
                                        next_endpoint_type next_endpoint,
                                        boost::system::error_code& ec) {
    return this->get_service().add_network(
        this->implementation, std::move(prefix), std::move(next_endpoint), ec);
  }

  template <class Handler>
  BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(const boost::system::error_code&))
      async_add_network(const prefix_type& prefix,
                        const next_endpoint_type& next_endpoint,
                        Handler handler) {
    return this->get_service().async_add_network(this->implementation, prefix,
                                                 next_endpoint, handler);
  }

  boost::system::error_code remove_network(const prefix_type& prefix,
                                           boost::system::error_code& ec) {
    return this->get_service().remove_network(this->implementation, prefix, ec);
  }

  ReceiveQueue* get_network_receive_queue(prefix_type prefix,
                                          boost::system::error_code& ec) {
    return this->get_service().get_network_receive_queue(this->implementation,
                                                         prefix, ec);
  }

  SendQueue* get_router_send_queue(boost::system::error_code& ec) {
    return this->get_service().get_router_send_queue(this->implementation, ec);
  }

  boost::system::error_code add_route(
      prefix_type prefix, network_address_type next_endpoint_context,
      boost::system::error_code& ec) {
    return this->get_service().add_route(this->implementation,
                                         std::move(prefix),
                                         std::move(next_endpoint_context), ec);
  }

  boost::system::error_code remove_route(prefix_type prefix,
                                         boost::system::error_code& ec) {
    return this->get_service().remove_route(this->implementation, prefix, ec);
  }

  network_address_type resolve(const prefix_type& prefix,
                               boost::system::error_code& ec) const {
    return this->get_service().resolve(this->implementation, prefix, ec);
  }

  template <class Handler>
  BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(const boost::system::error_code&,
                                              const network_address_type&))
      async_resolve(const prefix_type& prefix, Handler&& handler) {
    return this->get_service().async_resolve(this->implementation, prefix,
                                             std::forward<Handler>(handler));
  }

  template <class Handler>
  BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(const boost::system::error_code&,
                                              std::size_t))
      async_send(const SendDatagram& datagram, Handler handler) {
    return this->get_service().async_send(this->implementation, datagram,
                                          std::forward<Handler>(handler));
  }

  boost::system::error_code flush(boost::system::error_code& ec) {
    return this->get_service().flush(this->implementation, ec);
  }

  boost::system::error_code close(boost::system::error_code& ec) {
    return this->get_service().close(this->implementation, ec);
  }
};

}  // routing
}  // layer
}  // ssf

#endif  // SSF_LAYER_ROUTING_BASIC_ROUTER_H_
