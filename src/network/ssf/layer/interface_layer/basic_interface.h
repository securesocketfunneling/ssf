#ifndef SSF_LAYER_INTERFACE_BASIC_INTERFACE_H_
#define SSF_LAYER_INTERFACE_BASIC_INTERFACE_H_

#include <boost/asio/io_service.hpp>
#include <boost/asio/async_result.hpp>

#include <boost/system/error_code.hpp>

#include "basic_interface_service.h"

namespace ssf {
namespace layer {
namespace interface_layer {

template <class Protocol, class NextLayerProtocol,
          class InterfaceService =
              basic_Interface_service<Protocol, NextLayerProtocol>>
class basic_Interface : public boost::asio::basic_io_object<InterfaceService> {
 private:
  typedef typename InterfaceService::raw_next_layer_protocol
      raw_next_layer_protocol;
  typedef
      typename InterfaceService::endpoint_context_type endpoint_context_type;

 public:
  basic_Interface(boost::asio::io_service& io_service)
      : boost::asio::basic_io_object<InterfaceService>(io_service) {}

  basic_Interface(const basic_Interface& other) = delete;

  basic_Interface& operator=(const basic_Interface& other) = delete;

  basic_Interface(basic_Interface&& other)
      : boost::asio::basic_io_object<InterfaceService>(std::move(other)) {}

  basic_Interface& operator=(basic_Interface&& other) {
    boost::asio::basic_io_object<InterfaceService>::operator=(std::move(other));
    return *this;
  }

  ~basic_Interface() {
    boost::system::error_code ec;
    close(ec);
  }

  /// Async connect a next layer socket to its endpoint
  /// and register this socket as interface_name
  template <class ConnectHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ConnectHandler, void(boost::system::error_code))
      async_connect(endpoint_context_type interface_name,
                    const typename raw_next_layer_protocol::endpoint&
                        next_remote_endpoint,
                    ConnectHandler handler) {
    return this->get_service().async_connect(this->implementation,
                                             std::move(interface_name),
                                             next_remote_endpoint, handler);
  }

  /// Async accept connection of a next layer acceptor to its endpoint
  /// and register the first connection (next layer socket) as interface_name
  template <class AcceptHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(AcceptHandler, void(boost::system::error_code))
      async_accept(
          const endpoint_context_type& interface_name,
          const typename raw_next_layer_protocol::endpoint& next_local_endpoint,
          AcceptHandler handler) {
    return this->get_service().async_accept(
        this->implementation, interface_name, next_local_endpoint, handler);
  }

  boost::system::error_code close(boost::system::error_code& ec) {
    return this->get_service().close(this->implementation, ec);
  }

  bool is_open() { return this->get_service().is_open(this->implementation); }
};

}  // interface_layer
}  // layer
}  // ssf

#endif  // SSF_LAYER_INTERFACE_LAYER_BASIC_INTERFACE_H_
