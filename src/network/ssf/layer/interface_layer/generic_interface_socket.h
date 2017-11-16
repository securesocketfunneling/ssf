#ifndef SSF_LAYER_INTERFACE_LAYER_GENERIC_INTERFACE_SOCKET_H_
#define SSF_LAYER_INTERFACE_LAYER_GENERIC_INTERFACE_SOCKET_H_

#include <memory>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/asio/handler_type.hpp>

#include <boost/system/error_code.hpp>

#include "ssf/layer/io_handler.h"
#include "ssf/layer/interface_layer/interface_buffers.h"

namespace ssf {
namespace layer {
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

  virtual void close(boost::system::error_code& ec) = 0;

  virtual void connect(boost::system::error_code& ec) = 0;

  virtual void async_receive(interface_mutable_buffers buffers,
                             ssf::layer::WrappedIOHandler handler) = 0;

  virtual void async_send(interface_const_buffers buffers,
                          ssf::layer::WrappedIOHandler handler) = 0;
};

}  // interface_layer
}  // layer
}  // ssf

#endif  // SSF_LAYER_INTERFACE_LAYER_GENERIC_INTERFACE_SOCKET_H_
