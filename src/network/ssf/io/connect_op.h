#ifndef SSF_IO_CONNECT_OP_H_
#define SSF_IO_CONNECT_OP_H_

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

#include "ssf/io/op.h"

#include <boost/asio/detail/push_options.hpp>

namespace ssf {
namespace io {

/// Base class for pending connect operations
template <typename Protocol>
class basic_pending_connect_operation : public basic_pending_io_operation {
 protected:
  /// Constructor
  /**
  * @param func The completion handler
  */
  basic_pending_connect_operation(basic_pending_io_operation::func_type func)
      : basic_pending_io_operation(func) {}
};

/// Class to store connect operations
/**
* @tparam Handler The type of the handler to be called upon completion
* @tparam StreamSocket
*/
template <typename Handler, typename Protocol>
class pending_connect_operation
    : public basic_pending_connect_operation<Protocol> {
 public:
  BOOST_ASIO_DEFINE_HANDLER_PTR(pending_connect_operation);

  /// Constructor
  /**
  * @param handler The handler to call upon completion
  */
  pending_connect_operation(Handler handler)
      : basic_pending_connect_operation<Protocol>(
            &pending_connect_operation::do_complete),
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
    pending_connect_operation* o(static_cast<pending_connect_operation*>(base));

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
}  // ssf

#include <boost/asio/detail/pop_options.hpp>

#endif  // SSF_COMMON_IO_CONNECT_OP_H_
