//
// fiber/detail/io_fiber_accept_op.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2014-2015
//

#ifndef SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_IO_FIBER_ACCEPT_OP_HPP_
#define SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_IO_FIBER_ACCEPT_OP_HPP_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#include <boost/asio/detail/addressof.hpp>
#include <boost/asio/detail/bind_handler.hpp>
#include <boost/asio/detail/fenced_block.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/error.hpp>

#include "common/boost/fiber/detail/io_operation.hpp"
#include "common/boost/fiber/detail/fiber_header.hpp"

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace fiber {
namespace detail {

template <typename StreamSocket>
class basic_fiber_impl;

/// Class to store accept operations on a fiber
/**
* @tparam Handler The type of the handler to be called upon completion
* @tparam StreamSocket
*/
template <typename Handler, typename StreamSocket>
class pending_accept_operation
  : public basic_pending_accept_operation<StreamSocket>
{
private:
  typedef boost::asio::fiber::detail::basic_fiber_impl<StreamSocket> fiber_deref_impl;
  typedef std::shared_ptr<fiber_deref_impl> fiber_impl;

public:
  BOOST_ASIO_DEFINE_HANDLER_PTR(pending_accept_operation);

  /// Constructor
  /**
  * @param p_fib The fiber implementation of the accepted fiber
  * @param p_id The fiber id of the accepted fiber
  * @param handler The handler to call upon completion
  */
  pending_accept_operation(fiber_impl p_fib, fiber_id* p_id, Handler& handler)
    : basic_pending_accept_operation<StreamSocket>(
    &pending_accept_operation::do_complete, p_fib, p_id),
    handler_(BOOST_ASIO_MOVE_CAST(Handler)(handler))
  {
    }

  /// Implementation of the completion callback
  /**
  * @param base A pointer to the base class
  * @param destroy A boolean to decide if the op should be destroyed
  * @param result_ec The error_code of the operation
  * @param bytes_transferred (Unused) The number of bytes handled
  */
  static void do_complete(basic_pending_io_operation* base, bool destroy,
                          const boost::system::error_code& result_ec,
                          std::size_t /* bytes_transferred */)
  {
    boost::system::error_code ec(result_ec);

    // take ownership of the operation object
    pending_accept_operation* o(static_cast<pending_accept_operation*>(base));

    ptr p = { boost::asio::detail::addressof(o->handler_), o, o };

    BOOST_ASIO_HANDLER_COMPLETION((o));

    // Make a copy of the handler so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the handler may be the true owner of the memory associated
    // with the handler. Consequently, a local copy of the handler is required
    // to ensure that any owning sub-object remains valid until after we have
    // deallocated the memory here.
    boost::asio::detail::binder1<Handler, boost::system::error_code> handler(o->handler_,
                                                                              ec);
    p.h = boost::asio::detail::addressof(handler.handler_);
    p.reset();

    // Make the upcall if required.
    if (!destroy)
    {
      boost::asio::detail::fenced_block b(boost::asio::detail::fenced_block::half);
      BOOST_ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_));
      boost_asio_handler_invoke_helpers::invoke(handler, handler.handler_);
      BOOST_ASIO_HANDLER_INVOCATION_END;
    }
  }

private:
  Handler handler_;
};

} // namespace detail
} // namespace fiber
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif  // SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_IO_FIBER_ACCEPT_OP_HPP_
