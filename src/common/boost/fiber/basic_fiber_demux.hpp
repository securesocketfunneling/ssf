//
// fiber/basic_fiber_demux.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2014-2015
//

#ifndef SSF_COMMON_BOOST_ASIO_FIBER_BASIC_FIBER_DEMUX_HPP_
#define SSF_COMMON_BOOST_ASIO_FIBER_BASIC_FIBER_DEMUX_HPP_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif  // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <functional>

#include <boost/asio.hpp>

#include <ssf/log/log.h>

#include "common/boost/fiber/basic_fiber_demux_service.hpp"
#include "common/boost/fiber/detail/fiber_id.hpp"
#include "common/boost/fiber/detail/io_fiber_accept_op.hpp"

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace fiber {

namespace detail {
template <typename StreamSocket>
class basic_fiber_impl;
}  // namespace detail

template <typename StreamSocket,
          typename Service = basic_fiber_demux_service<StreamSocket>>
class basic_fiber_demux : private boost::noncopyable {
 public:
  /// Type of the socket over which to demultiplex
  typedef StreamSocket socket_type;

  /// Type of the service running the demux
  typedef Service service_type;

  /// Type of the implementation class of the fiber demux.
  typedef typename Service::implementation_deref_type implementation_deref_type;

  /// Type of a pointer to the implementation class of the fiber demux.
  typedef typename Service::implementation_type implementation_type;

  /// Type of local ports used for demultiplexing fibers
  typedef typename Service::local_port_type local_port_type;

  /// Type of remote ports used for demultiplexing fibers
  typedef typename Service::remote_port_type remote_port_type;

  /// Type of the implementation class of the fiber.
  typedef typename Service::fiber_impl_deref_type fiber_impl_deref_type;

  /// Type of a pointer to the implementation class of the fiber.
  typedef typename Service::fiber_impl_type fiber_impl_type;

  /// Type of the object identifying a fiber (with a remote and a local fiber
  /// port)
  typedef typename Service::fiber_id fiber_id;

 private:
  typedef boost::asio::fiber::detail::basic_pending_accept_operation<
      StreamSocket>
      accept_op;
  typedef std::function<void()> close_handler_type;

 public:
  /// Construct a basic_fiber_demux object
  /**
    * This constructor creates a basic_fiber_demux object without any socket to
    * demultiplex
    *
    * @param io_service The io_service object that the fiber will use to
    * dispatch handlers for any asynchronous operations performed on it.
    */
  explicit basic_fiber_demux(boost::asio::io_service& io_service)
      : service_(boost::asio::use_service<service_type>(io_service)),
        impl_(nullptr) {}

  ~basic_fiber_demux() { SSF_LOG("demux", debug, "destroy"); }

  /// Return the io_service managing the fiber demux.
  /**
  * This function is used to recover the io_service managing asynchronous
  * operations of the fiber demux.
  */
  boost::asio::io_service& get_io_service() {
    return service_.get_io_service();
  }

  StreamSocket& socket() { return impl_->socket; }

  /// Start demultiplexing the stream socket
  /**
  * This function is used to initiate the demultiplexing on the stream socket
  *
  * @param socket The stream socket on which to demultiplex.
  *
  * @note Once this function has been called, the socket should not be directly
  * read from or written to by the user.
  */
  void fiberize(StreamSocket socket, close_handler_type close = []() {},
                size_t mtu = 60 * 1024) {
    if (mtu > 60 * 1024) {
      SSF_LOG("demux", warn, "MTU too big, replaced with MAX_VALUE");
      mtu = 60 * 1024;
    }

    impl_ = implementation_deref_type::create(std::move(socket), close, mtu);
    service_.fiberize(impl_);
  }

  /// Bind a fiber acceptor to the given local fiber port.
  /**
  * This function binds a fiber acceptor to the specified local fiber port
  * on the io_fiber demultiplexer.
  *
  * @param local_port A local fiber port to which the fiber acceptor will be
  *bound.
  *
  * @param fib_impl The fiber acceptor implementation object.
  *
  * @param ec Set to indicate what error occurred, if any.
  *
  */
  void bind(local_port_type local_port, fiber_impl_type fib_impl,
            boost::system::error_code& ec) {
    service_.bind(impl_, local_port, fib_impl, ec);
  }

  /// Open a fibert local port to be listened on
  /**
  * This function puts the local fiber port into the state where it may accept
  * new connections.
  *
  * @param local_port The local fiber port.
  * @param ec Set to indicate what error occurred, if any.
  */
  void listen(local_port_type local_port, boost::system::error_code& ec) {
    service_.listen(impl_, local_port, ec);
  }

  /// Asynchronously starts an acknowledgement for a received connection
  /**
  * This function starts the acknowledgement for a received connection to
  * complete the fiber connection handshake.
  *
  * @param id A reference to the fiber id
  *
  * @param fib_impl The implementation object of the fiber being connected
  */
  void async_send_ack(fiber_impl_type fib_impl, accept_op* op) {
    service_.async_send_ack(impl_, fib_impl, op);
  }

  /// Start an asynchronous connect.
  /**
  * This function is used to asynchronously connect a
  * fiber. The function call always returns immediately.
  *
  * @param remote_port The remote fiber port to which the fiber should be
  *connected.
  *
  * @param fib_impl The implementation object of the fiber.
  *
  * @note When the connection process is finished, the handler
  *connect_user_handler
  * of the implementation object will be called.
  */
  void async_connect(remote_port_type remote_port, fiber_impl_type fib_impl) {
    service_.async_connect(impl_, remote_port, fib_impl);
  }

  /// Start an asynchronous receive.
  /**
  * This function is used to asynchronously receive data from a basic
  * fiber. The function call always returns immediately.
  *
  * @param buffers One or more buffers into which the data will be received.
  * Although the buffers object may be copied as necessary, ownership of the
  * underlying memory blocks is retained by the caller, which must guarantee
  * that they remain valid until the handler is called.
  *
  * @param handler The handler to be called when the receive operation
  * completes. Copies will be made of the handler as required. The function
  * signature of the handler must be:
  * @code void handler(
  *   const boost::system::error_code& error, // Result of operation.
  *   std::size_t bytes_transferred           // Number of bytes received.
  * ); @endcode
  * Regardless of whether the asynchronous operation completes immediately or
  * not, the handler will not be invoked from within this function. Invocation
  * of the handler will be performed in a manner equivalent to using
  * boost::asio::io_service::post().
  */
  template <typename MutableBufferSequence, typename Handler>
  void async_receive(const MutableBufferSequence& buffers, Handler& handler) {
    return service_.async_receive(impl_, buffers, handler);
  }

  template <typename ConstBufferSequence, typename Handler>
  void async_send(const ConstBufferSequence& buffers, fiber_id id,
                  Handler& handler) {
    return service_.async_send_data(impl_, id, buffers, handler);
  }

  template <typename ConstBufferSequence, typename Handler>
  void async_send_dgr(const ConstBufferSequence& buffers,
                      remote_port_type remote_port, fiber_impl_type fib_impl,
                      Handler& handler) {
    return service_.async_send_datagram(impl_, remote_port, fib_impl, buffers,
                                        handler);
  }

  /// Close fiber.
  /**
  * This closes a fiber immediatly. It cancels all pending operations from this
  * fiber.
  */
  void close_fiber(fiber_impl_type fib_impl) {
    service_.close_fiber(impl_, fib_impl);
  }

  /// Close the demultiplexer (does not close the underlaying socket)
  /**
  * This synchronously close the fiber demux.
  */
  void close() {
    if (impl_.get()) {
      service_.close(impl_);
    }
  }

 private:
  service_type& service_;
  implementation_type impl_;
};
}  // namespace fiber
}  // namespace asio
}  // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif  // SSF_COMMON_BOOST_ASIO_FIBER_BASIC_FIBER_DEMUX_HPP_
