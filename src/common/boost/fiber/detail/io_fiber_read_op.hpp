//
// fiber/detail/io_fiber_read_op.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2014-2015
//

#ifndef SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_IO_FIBER_READ_OP_HPP_
#define SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_IO_FIBER_READ_OP_HPP_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#include <boost/asio/detail/addressof.hpp>
#include <boost/asio/detail/bind_handler.hpp>
#include <boost/asio/detail/buffer_sequence_adapter.hpp>
#include <boost/asio/detail/fenced_block.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/detail/operation.hpp>
#include <boost/asio/detail/socket_ops.hpp>
#include <boost/asio/error.hpp>

#include "common/boost/fiber/detail/io_operation.hpp"
#include "common/boost/fiber/detail/fiber_id.hpp"

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace fiber {
namespace detail {

/// Class to store buffers and read operations on a fiber
/**
* @tparam MutableBufferSequence The type of the sequence of mutable buffers used
* @tparam Handler The type of the handler to be called upon completion
*/
template <typename MutableBufferSequence, typename Handler>
class pending_read_operation : public basic_pending_read_operation
{
public:
  BOOST_ASIO_DEFINE_HANDLER_PTR(pending_read_operation);

  /// Constructor
  /**
  * @param buffers The reading buffers
  * @param handler The handler to call upon completion
  */
  pending_read_operation(const MutableBufferSequence& buffers, Handler& handler)
    : basic_pending_read_operation(&pending_read_operation::do_complete,
                                   &pending_read_operation::do_fill_buffers,
                                   nullptr),
    buffers_(buffers),
    handler_(handler)
  {
  }

  /// Constructor
  /**
  * @param buffers The reading buffers
  * @param handler The handler to call upon completion
  * @param p_remote_port A pointer to the remote port from where the data was
  * received
  */
  pending_read_operation(const MutableBufferSequence& buffers,
                         Handler& handler,
                         fiber_id::remote_port_type* p_remote_port)
                         : basic_pending_read_operation(&pending_read_operation::do_complete,
                                                        &pending_read_operation::do_fill_buffers,
                                                        p_remote_port),
                           buffers_(buffers),
                           handler_(handler)
  {
  }

  /// Implementation of the completion callback
  /**
  * @param base A pointer to the base class
  * @param destroy A boolean to decide if the op should be destroyed
  * @param result_ec The error_code of the operation
  * @param bytes_transferred The number of bytes handled
  */
  static void do_complete(basic_pending_io_operation* base, bool destroy,
                          const boost::system::error_code& result_ec,
                          std::size_t bytes_transferred)
  {
    boost::system::error_code ec(result_ec);

    // Take ownership of the operation object.
    pending_read_operation* o(static_cast<pending_read_operation*>(base));

    ptr p = { boost::asio::detail::addressof(o->handler_), o, o };

    BOOST_ASIO_HANDLER_COMPLETION((o));

    // Make a copy of the handler so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the handler may be the true owner of the memory associated
    // with the handler. Consequently, a local copy of the handler is required
    // to ensure that any owning sub-object remains valid until after we have
    // deallocated the memory here.
    boost::asio::detail::binder2<Handler,
                                 boost::system::error_code,
                                 std::size_t> handler(
      o->handler_, ec, bytes_transferred);
    p.h = boost::asio::detail::addressof(handler.handler_);
    p.reset();

    // Make the upcall if required.
    if (!destroy)
    {
      boost::asio::detail::fenced_block b(boost::asio::detail::fenced_block::half);
      BOOST_ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_, handler.arg2_));
      boost_asio_handler_invoke_helpers::invoke(handler, handler.handler_);
      BOOST_ASIO_HANDLER_INVOCATION_END;
    }
  }

  /// Implementation of the filling buffer callback
  /**
  * @param base A pointer to the base class
  * @param stream The buffer to copy the data to
  */
  static size_t do_fill_buffers(basic_pending_io_operation* base,
                                boost::asio::streambuf& stream)
  {
    pending_read_operation* o(static_cast<pending_read_operation*>(base));

    size_t copied = 0;

    for (auto it_buffer = o->buffers_.begin(); it_buffer != o->buffers_.end();
          ++it_buffer)
    {
      size_t just_copied = boost::asio::buffer_copy(*it_buffer, stream.data());
      stream.consume(just_copied);
      copied += just_copied;
      if (!stream.size())
      {
        break;
      }
    }

    return copied;
  }

private:
  boost::asio::detail::consuming_buffers<mutable_buffer, MutableBufferSequence> buffers_;
  Handler handler_;
};

} // namespace detail
} // namespace fiber
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif  // SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_IO_FIBER_READ_OP_HPP_
