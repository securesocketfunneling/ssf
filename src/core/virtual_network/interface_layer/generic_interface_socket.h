#ifndef SSF_CORE_VIRTUAL_NETWORK_INTERFACE_LAYER_GENERIC_INTERFACE_SOCKET_H_
#define SSF_CORE_VIRTUAL_NETWORK_INTERFACE_LAYER_GENERIC_INTERFACE_SOCKET_H_

#include <memory>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/asio/handler_type.hpp>

#include <boost/system/error_code.hpp>

#include "core/virtual_network/io_handler.h"
#include "core/virtual_network/interface_layer/interface_buffers.h"

namespace virtual_network {
namespace interface_layer {

template <class Protocol>
class generic_interface_socket {
 public:
  typedef Protocol protocol_type;

 private:
  typedef typename protocol_type::endpoint endpoint_type;

 public:
  virtual std::size_t available(boost::system::error_code& ec) = 0;

  virtual boost::system::error_code cancel(boost::system::error_code& ec) = 0;

  virtual void async_receive(interface_mutable_buffers buffers,
                             virtual_network::WrappedIOHandler handler) = 0;

  virtual void async_send(interface_const_buffers buffers,
                          virtual_network::WrappedIOHandler handler) = 0;
};

}  // interface_layer
}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_INTERFACE_LAYER_GENERIC_INTERFACE_SOCKET_H_
