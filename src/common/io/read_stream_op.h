#ifndef SSF_COMMON_IO_READ_STREAM_OP_H_
#define SSF_COMMON_IO_READ_STREAM_OP_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif  // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstdint>

#include <boost/asio/detail/config.hpp>

#include <boost/asio/basic_socket.hpp>
#include <boost/asio/detail/addressof.hpp>
#include <boost/asio/detail/bind_handler.hpp>
#include <boost/asio/detail/fenced_block.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/streambuf.hpp>

#include "common/io/op.h"

#include <boost/asio/detail/push_options.hpp>

namespace io {

/// Base class for pending read operations
class basic_pending_read_stream_operation
    : public basic_pending_sized_io_operation {
 protected:
  typedef size_t (*fill_buffer_func_type)(basic_pending_read_stream_operation*,
                                          boost::asio::streambuf&);

 protected:
  /// Constructor
  /**
  * @param func The completion handler
  */
  basic_pending_read_stream_operation(
      basic_pending_sized_io_operation::func_type func,
      fill_buffer_func_type fill_buffer_func)
      : basic_pending_sized_io_operation(func),
        fill_buffer_func_(fill_buffer_func) {}

 public:
  size_t fill_buffer(boost::asio::streambuf& stream) {
    return fill_buffer_func_(this, stream);
  }

 private:
  fill_buffer_func_type fill_buffer_func_;
};

/// Class to store read operations
/**
* @tparam Handler The type of the handler to be called upon completion
*/
template <class MutableBufferSequence, class Handler>
class pending_read_stream_operation
    : public basic_pending_read_stream_operation {
 public:
  BOOST_ASIO_DEFINE_HANDLER_PTR(pending_read_stream_operation);

  /// Constructor
  /**
  * @param handler The handler to call upon completion
  */
  pending_read_stream_operation(const MutableBufferSequence& buffers,
                                Handler handler)
      : basic_pending_read_stream_operation(
            &pending_read_stream_operation::do_complete,
            &pending_read_stream_operation::do_fill_buffer),
        buffers_(buffers),
        handler_(std::move(handler)) {}

  /// Implementation of the completion callback
  /**
  * @param base A pointer to the base class
  * @param destroy A boolean to decide if the op should be destroyed
  * @param result_ec The error_code of the operation
  */
  static void do_complete(basic_pending_sized_io_operation* base, bool destroy,
                          const boost::system::error_code& result_ec,
                          std::size_t length) {
    boost::system::error_code ec(result_ec);

    // take ownership of the operation object
    pending_read_stream_operation* o(
        static_cast<pending_read_stream_operation*>(base));

    ptr p = {boost::asio::detail::addressof(o->handler_), o, o};

    BOOST_ASIO_HANDLER_COMPLETION((o));

    // Make a copy of the handler so that the memory can be deallocated before
    // the upcall is made. Even if we're not about to make an upcall, a
    // sub-object of the handler may be the true owner of the memory associated
    // with the handler. Consequently, a local copy of the handler is required
    // to ensure that any owning sub-object remains valid until after we have
    // deallocated the memory here.
    boost::asio::detail::binder2<Handler, boost::system::error_code,
                                 std::size_t> handler(o->handler_, ec, length);
    p.h = boost::asio::detail::addressof(handler.handler_);
    p.reset();

    // Make the upcall if required.
    if (!destroy) {
      boost::asio::detail::fenced_block b(
          boost::asio::detail::fenced_block::half);
      BOOST_ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_, handler.arg2_));
      boost_asio_handler_invoke_helpers::invoke(handler, handler.handler_);
      BOOST_ASIO_HANDLER_INVOCATION_END;
    }
  }

  static size_t do_fill_buffer(basic_pending_read_stream_operation* base,
                               boost::asio::streambuf& stream) {
    pending_read_stream_operation* o(
        static_cast<pending_read_stream_operation*>(base));

    auto copied = boost::asio::buffer_copy(o->buffers_, stream.data());
    stream.consume(copied);

    return copied;
  }

 private:
  MutableBufferSequence buffers_;
  Handler handler_;
};

}  // io

#include <boost/asio/detail/pop_options.hpp>

#endif  // SSF_COMMON_IO_READ_STREAM_OP_H_
