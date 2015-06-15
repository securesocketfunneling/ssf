#ifndef SSF_COMMON_IO_ACCEPT_OP_H_
#define SSF_COMMON_IO_ACCEPT_OP_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif  // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#include <boost/asio/basic_socket.hpp>
#include <boost/asio/detail/addressof.hpp>
#include <boost/asio/detail/bind_handler.hpp>
#include <boost/asio/detail/fenced_block.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/error.hpp>

#include "common/io/op.h"

#include <boost/asio/detail/push_options.hpp>

namespace io {

/// Base class for pending fiber accept operations
template <typename Protocol>
class basic_pending_accept_operation : public basic_pending_io_operation {
 protected:
  typedef boost::asio::basic_socket<typename Protocol::socket::protocol_type,
                                    typename Protocol::socket::service_type>
      socket_type;
  typedef typename Protocol::endpoint endpoint_type;

 protected:
  /// Constructor
  /**
  * @param func The completion handler
  * @param impl The implementation of the accepted socket
  * @param p_endpoint The remote endpoint of the accepted socket
  */
  basic_pending_accept_operation(basic_pending_io_operation::func_type func,
                                 socket_type& peer, endpoint_type* p_endpoint)
      : basic_pending_io_operation(func),
        peer_(peer),
        p_endpoint_(p_endpoint) {}

 public:
  /// Set the remote endpoint of the accepted socket
  /**
  * @param e The remote endpoint to set
  */
   void set_p_endpoint(const endpoint_type& e) { 
     if (p_endpoint_) {
       *p_endpoint_ = e;
     }
   }

  /// Get the accepted socket implementation
   socket_type& peer() { return peer_; }

 private:
  socket_type& peer_;
  endpoint_type* p_endpoint_;
};

/// Class to store accept operations on a fiber
/**
* @tparam Handler The type of the handler to be called upon completion
* @tparam StreamSocket
*/
template <typename Handler, typename Protocol>
class pending_accept_operation
    : public basic_pending_accept_operation<Protocol> {
private:
  typedef typename basic_pending_accept_operation::socket_type
    socket_type;
 typedef typename basic_pending_accept_operation::endpoint_type endpoint_type;

 public:
  BOOST_ASIO_DEFINE_HANDLER_PTR(pending_accept_operation);

  /// Constructor
  /**
  * @param impl The socket implementation of the accepted socket
  * @param p_endpoint The remote endpoint of the accepted socket
  * @param handler The handler to call upon completion
  */
  pending_accept_operation(socket_type& peer, endpoint_type* p_endpoint,
                           Handler handler)
      : basic_pending_accept_operation<Protocol>(
            &pending_accept_operation::do_complete, peer, p_endpoint),
        handler_(std::move(handler)) {}

  /// Implementation of the completion callback
  /**
  * @param base A pointer to the base class
  * @param destroy A boolean to decide if the op should be destroyed
  * @param result_ec The error_code of the operation
  */
  static void do_complete(basic_pending_io_operation* base, bool destroy,
                          const boost::system::error_code& result_ec) {
    boost::system::error_code ec(result_ec);

    // take ownership of the operation object
    pending_accept_operation* o(static_cast<pending_accept_operation*>(base));

    ptr p = {boost::asio::detail::addressof(o->handler_), o, o};

    BOOST_ASIO_HANDLER_COMPLETION((o));

    // Make a copy of the handler so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the handler may be the true owner of the memory associated
    // with the handler. Consequently, a local copy of the handler is required
    // to ensure that any owning sub-object remains valid until after we have
    // deallocated the memory here.
    boost::asio::detail::binder1<Handler, boost::system::error_code> handler(
        o->handler_, ec);
    p.h = boost::asio::detail::addressof(handler.handler_);
    p.reset();

    // Make the upcall if required.
    if (!destroy) {
      boost::asio::detail::fenced_block b(
          boost::asio::detail::fenced_block::half);
      BOOST_ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_));
      boost_asio_handler_invoke_helpers::invoke(handler, handler.handler_);
      BOOST_ASIO_HANDLER_INVOCATION_END;
    }
  }

 private:
  Handler handler_;
};

}  // io

#include <boost/asio/detail/pop_options.hpp>

#endif  // SSF_COMMON_IO_ACCEPT_OP_H_
