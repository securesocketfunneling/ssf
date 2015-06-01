//
// fiber/basic_fiber_demux_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2014-2015
//

#ifndef SSF_COMMON_BOOST_ASIO_FIBER_BASIC_FIBER_DEMUX_SERVICE_HPP_
#define SSF_COMMON_BOOST_ASIO_FIBER_BASIC_FIBER_DEMUX_SERVICE_HPP_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/detail/workaround.hpp>

#include "common/boost/fiber/detail/basic_fiber_demux_impl.hpp"
#include "common/boost/fiber/detail/fiber_id.hpp"
#include "common/boost/fiber/detail/fiber_buffer.hpp"
#include "common/boost/fiber/detail/io_fiber_accept_op.hpp"

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace fiber {

namespace detail {
  template <typename StreamSocket>
  class basic_fiber_impl;
} // namespace detail

/// Class which implements all the demultiplexing abilities in an asio service
template <typename StreamSocket>
class basic_fiber_demux_service
#if defined(GENERATING_DOCUMENTATION)
  : public boost::asio::io_service::service
#else
  : public boost::asio::detail::service_base<
  basic_fiber_demux_service<StreamSocket> >
#endif
{
private:
  /// The current service type.
  typedef basic_fiber_demux_service<StreamSocket> service;

public:
  /// Type of the implementation class of the fiber.
  typedef boost::asio::fiber::detail::basic_fiber_impl<StreamSocket>
    fiber_impl_deref_type;

  /// Type of a pointer to the implementation class of the fiber.
  typedef std::shared_ptr<fiber_impl_deref_type> fiber_impl_type;

  /// Type of a remote fiber port.
  typedef boost::asio::fiber::detail::fiber_id::remote_port_type remote_port_type;

  /// Type of a local fiber port.
  typedef boost::asio::fiber::detail::fiber_id::local_port_type local_port_type;

  /// Type for the id class identifying fibers.
  typedef boost::asio::fiber::detail::fiber_id fiber_id;

  /// Type for the buffer class used to receive packets from a fiber.
  typedef boost::asio::fiber::detail::fiber_buffer fiber_buffer;

  /// Type of a pointer to the fiber buffer class
  typedef boost::asio::fiber::detail::p_fiber_buffer p_fiber_buffer;

  /// Type of the headers of fiber packets.
  typedef boost::asio::fiber::detail::fiber_header fiber_header;

  /// Type of the flag field in the headers of fiber packets.
  typedef boost::asio::fiber::detail::fiber_header::flags_type flag_type;

  /// Type of a class used to store accepting operations (on a fiber acceptor).
  typedef boost::asio::fiber::detail::basic_pending_accept_operation<StreamSocket>
    accept_op;

public:
#if defined(GENERATING_DOCUMENTATION)
  static boost::asio::io_service::id id;
#endif

  /// Type of the implementation class of the fiber demux.
  typedef boost::asio::fiber::detail::basic_fiber_demux_impl<StreamSocket>
    implementation_deref_type;

  /// Type of a pointer to the implementation class of the fiber demux.
  typedef std::shared_ptr<implementation_deref_type> implementation_type;

public:
  /// Construct a basic_fiber_demux_service object
  /**
  * This constructor creates a basic_fiber_demux_service object.
  *
  * @param io_service The io_service object that the fibers will use to
  * dispatch handlers for any asynchronous operations performed on it.
  */
  explicit basic_fiber_demux_service(boost::asio::io_service& io_service)
    : boost::asio::detail::service_base<service>(io_service),
    io_service_(io_service)
  {
  }

  /// Destructor
  ~basic_fiber_demux_service() {}


  /// Get the io_service in charge of all asynchronous operations.
  boost::asio::io_service& get_io_service() { return io_service_; }

  /// Start to demultiplex with a given demux
  /**
  * @param impl A pointer to the implementation of a demux.
  */
  void fiberize(implementation_type impl);

  /// Bind a fiber to a fiber id and a demux
  /**
  * @param impl A pointer to the implementation of a demux.
  * @param id A reference to a fiber id to which the fiber will be bound.
  * @param fib_impl A pointer to the implementation of the fiber to be bound.
  * @param ec The error code object in which any error will be stored.
  */
  void bind(implementation_type impl, local_port_type local_port,
            fiber_impl_type fib_impl, boost::system::error_code& ec);

  /// Check if a there is a fiber bound to a given demux and fiber id.
  /**
  * @param impl A pointer to the implementation of a demux.
  * @param id A reference to a fiber id.
  */
  bool is_bound(implementation_type impl, const fiber_id& id);

  /// Unbind any fiber bound to a given demux and id.
  /**
  * @param impl A pointer to the implementation of a demux.
  * @param id A reference to a fiber id.
  */
  void unbind(implementation_type impl, const fiber_id& id);

  /// Notify the service that a fiber acceptor is listening on a fiber port.
  /**
  * @param impl A pointer to the implementation of a demux.
  * @param local_port The port on which the fiber acceptor is listening.
  * @param ec The error code object in which any error will be stored.
  */
  void listen(implementation_type impl, local_port_type local_port,
              boost::system::error_code& ec);

  /// Check if a fiber acceptor is listening on a given demux and local port.
  /**
  * @param impl A pointer to the implementation of a demux.
  * @param local_port The port on which the fiber acceptor is listening.
  */
  bool is_listening(implementation_type impl, local_port_type local_port);

  /// Stop a fiber acceptor from listening on a given demux and local port.
  /**
  * @param impl A pointer to the implementation of a demux.
  * @param local_port The port on which the fiber acceptor is listening.
  */
  void stop_listening(implementation_type impl, local_port_type local_port);

  /// Start an asynchronous connect.
  /**
  * This function is used to asynchronously connect the fiber through a demux to
  * a remote fiber port.
  *
  * @param impl A pointer to the implementation of the demux.
  * @param remote_port The remote fiber port to connect to.
  * @param fib_impl A pointer to the implementation of the fiber to connect.
  */
  void async_connect(implementation_type impl, remote_port_type remote_port,
                      fiber_impl_type fib_impl);

  /// Start an asynchronous send of user data.
  /**
  * This function is used to asynchronously send data through the fiber bound to
  * the given demux and fiber id.
  *
  * @param impl A pointer to the implementation of the demux.
  * @param id The fiber id.
  * @param buffers The user buffers to send.
  * @param handler The user handler to execute when the data is sent.
  */
  template <typename ConstBufferSequence, typename Handler>
  void async_send_data(implementation_type impl, fiber_id id, ConstBufferSequence& buffers,
                        Handler& handler)
  {
    async_send_push(impl, id, buffers, handler);
  }

  /// Start an asynchronous send of user datagram.
  /**
  * This function is used to asynchronously send a datagram through the datagram
  * fiber bound to the given demux and remote port.
  *
  * @param impl A pointer to the implementation of the demux.
  * @param remote_port the remote port to which the datagram should be sent.
  * @param fib_impl A pointer to the implementation of the datagram fiber.
  * @param buffers The user buffers to send.
  * @param handler The user handler to execute when the data is sent.
  */
  template <typename ConstBufferSequence, typename Handler>
  void async_send_datagram(implementation_type impl, remote_port_type remote_port,
                            fiber_impl_type fib_impl,
                            ConstBufferSequence& buffers, Handler& handler)
  {
    async_send_dgr(impl, remote_port, fib_impl, buffers, handler);
  }

  void close_fiber(implementation_type impl, fiber_impl_type fib_impl);

  void close(implementation_type impl);

  void shutdown_service();

  void async_send_ack(implementation_type impl, fiber_impl_type fib_impl, accept_op* op);

private:
  enum
  {
    kFlagSyn = 1,
    kFlagReset = 2,
    kFlagAck = 4,
    kFlagDatagram = 8,
    kFlagPush = 16
  };
  void async_poll_packets(implementation_type impl);
  template<typename Handler>
  void async_send_rst(implementation_type impl, fiber_id id, const Handler& handler);
  void async_send_syn(implementation_type impl, fiber_id id);

  template <typename ConstBufferSequence, typename Handler>
  void async_send_push(implementation_type impl, fiber_id id, ConstBufferSequence& buffer,
                        Handler& handler);

  template <typename ConstBufferSequence, typename Handler>
  void async_send_dgr(implementation_type impl, remote_port_type remote_port,
                      fiber_impl_type fib_impl, ConstBufferSequence& buffer,
                      Handler& handler);

  void handle_dgr(implementation_type impl, p_fiber_buffer p_fiber_buff);
  void handle_push(implementation_type impl, p_fiber_buffer p_fiber_buff);
  void handle_ack(implementation_type impl, p_fiber_buffer p_fiber_buff);
  void handle_syn(implementation_type impl, p_fiber_buffer p_fiber_buff);
  void handle_rst(implementation_type impl, p_fiber_buffer p_fiber_buff);

  void async_push_packets(implementation_type impl);
  void dispatch_buffer(implementation_type impl, p_fiber_buffer p_fiber_buff);

  local_port_type get_available_local_port(implementation_type impl);

  template <typename ConstBufferSequence, typename Handler>
  void async_send(implementation_type impl, fiber_id id, flag_type flags,
                  ConstBufferSequence& buffers, Handler handler,
                  uint8_t priority = 0);

  template <typename ConstBufferSequence>
  std::vector<boost::asio::const_buffer> get_partial_buffer_sequence(
    ConstBufferSequence buffers, size_t length);

  void close_all_fibers(implementation_type impl);

private:
  io_service& io_service_;
  boost::random::mt19937 gen_;
};

} // namespace fiber
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#include "common/boost/fiber/basic_fiber_demux_service.ipp"

#endif  // SSF_COMMON_BOOST_ASIO_FIBER_BASIC_FIBER_DEMUX_SERVICE_HPP_
