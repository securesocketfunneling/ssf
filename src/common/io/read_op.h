#ifndef SSF_COMMON_IO_READ_OP_H_
#define SSF_COMMON_IO_READ_OP_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif  // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>

#include <boost/asio/detail/addressof.hpp>
#include <boost/asio/detail/bind_handler.hpp>
#include <boost/asio/detail/fenced_block.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/buffer.hpp>

#include "common/io/op.h"

#include <boost/asio/detail/push_options.hpp>

namespace io {

/// Base class for pending read operations
template <typename Protocol>
class basic_pending_read_operation : public basic_pending_sized_io_operation {
 protected:
  typedef typename Protocol::endpoint endpoint_type;
  typedef std::size_t (*fill_buffer_func_type)(
      basic_pending_read_operation*, typename Protocol::ReceiveDatagram&);

 protected:
  /// Constructor
  /**
  * @param func The completion handler
  * @param fill_buffer_func The fill buffer handler
  * @param p_endpoint The remote endpoint
  */
  basic_pending_read_operation(basic_pending_sized_io_operation::func_type func,
                               fill_buffer_func_type fill_buffer_func,
                               endpoint_type* p_endpoint)
      : basic_pending_sized_io_operation(func),
        p_endpoint_(p_endpoint),
        fill_buffer_func_(fill_buffer_func) {}

 public:
  /// Set the remote endpoint of the accepted socket
  /**
  * @param e The remote endpoint to set
  */
  void set_p_endpoint(endpoint_type e) {
    if (p_endpoint_) {
      *p_endpoint_ = std::move(e);
    }
  }

  std::size_t fill_buffer(typename Protocol::ReceiveDatagram& datagram) {
    return fill_buffer_func_(this, datagram);
  }

 private:
  endpoint_type* p_endpoint_;
  fill_buffer_func_type fill_buffer_func_;
};

/// Class to store read operations
/**
* @tparam MutableBufferSequence The type of the mutable buffer sequence
* @tparam Handler The type of the handler to be called upon completion
* @tparam Protocol Protocol of the read op 
*/
template <class MutableBufferSequence, class Handler, class Protocol>
class pending_read_operation : public basic_pending_read_operation<Protocol> {
 private:
  typedef typename basic_pending_read_operation<Protocol>::endpoint_type endpoint_type;

 public:
  BOOST_ASIO_DEFINE_HANDLER_PTR(pending_read_operation);

  /// Constructor
  /**
  * @param buffers The buffers to fill in
  * @param handler The handler to call upon completion
  * @param p_endpoint The remote endpoint
  */
  pending_read_operation(const MutableBufferSequence& buffers, Handler handler,
                         endpoint_type* p_endpoint)
      : basic_pending_read_operation<Protocol>(
            &pending_read_operation::do_complete,
            &pending_read_operation::do_fill_buffer, p_endpoint),
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
    pending_read_operation* o(static_cast<pending_read_operation*>(base));

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

  static std::size_t do_fill_buffer(
      basic_pending_read_operation<Protocol>* base,
      typename Protocol::ReceiveDatagram& datagram) {
    pending_read_operation* o(static_cast<pending_read_operation*>(base));

    if (boost::asio::buffer_size(o->buffers_) < datagram.payload().GetSize()) {
      return std::numeric_limits<std::size_t>::max();
    }

    auto payload_buffers = datagram.payload().GetConstBuffers();

    std::size_t copied = boost::asio::buffer_copy(o->buffers_, payload_buffers);

    return copied;
  }

 private:
  MutableBufferSequence buffers_;
  Handler handler_;
};

}  // io

#include <boost/asio/detail/pop_options.hpp>

#endif  // SSF_COMMON_IO_READ_OP_H_
