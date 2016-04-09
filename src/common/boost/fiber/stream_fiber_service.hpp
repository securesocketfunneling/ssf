//
// fiber/stream_fiber_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2014-2015
//

#ifndef SSF_COMMON_BOOST_ASIO_FIBER_STREAM_FIBER_SERVICE_HPP_
#define SSF_COMMON_BOOST_ASIO_FIBER_STREAM_FIBER_SERVICE_HPP_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstddef>

#include <memory>
#include <functional>

#include <boost/asio/detail/config.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/error.hpp>
#include <boost/log/trivial.hpp>

#include "common/boost/fiber/detail/io_fiber_read_op.hpp"

#include "common/boost/fiber/basic_fiber_demux.hpp"
#include "common/boost/fiber/detail/fiber_id.hpp"
#include "common/boost/fiber/basic_endpoint.hpp"
#include "common/boost/fiber/detail/basic_fiber_impl.hpp"

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace fiber {

/// Default service implementation for fiber
template <typename Protocol>
class stream_fiber_service
#if defined(GENERATING_DOCUMENTATION)
  : public boost::asio::io_service::service
#else
  : public boost::asio::detail::service_base<stream_fiber_service<Protocol> >
#endif
{
#if defined(GENERATING_DOCUMENTATION)
  /// The unique service identifier.
  static boost::asio::io_service::id id;
#endif

public:
  /// The protocol type.
  typedef Protocol protocol_type;

  /// The endpoint type.
  typedef typename Protocol::endpoint endpoint_type;

  /// Type of the demultiplexer.
  typedef typename Protocol::demux_type demux_type;

  /// Deref of the implementation type
  typedef boost::asio::fiber::detail::basic_fiber_impl
    <typename Protocol::socket_type> implementation_deref_type;

  /// Implementation type
  typedef std::shared_ptr<implementation_deref_type> implementation_type;

  /// (Deprecated: Use native_handle_type.) The native socket type.
#if defined(GENERATING_DOCUMENTATION)
  typedef implementation_defined native_type;
#else
  typedef implementation_type native_type;
#endif

  /// The native socket type.
#if defined(GENERATING_DOCUMENTATION)
  typedef implementation_defined native_handle_type;
#else
  typedef implementation_type native_handle_type;
#endif

public:
  /// Construct a stream_fiber_service object
  /**
  * This constructor creates a stream_fiber_service object.
  *
  * @param io_service The io_service object that the fibers will use to
  * dispatch handlers for any asynchronous operations performed on it.
  */
  explicit stream_fiber_service(boost::asio::io_service& io_service)
    : boost::asio::detail::service_base<stream_fiber_service<Protocol>>(io_service) {}

  /// Construct a new fiber implementation.
  void construct(implementation_type& impl)
  {
    impl = implementation_deref_type::create();
  }

  void move_construct(implementation_type& impl, implementation_type& other) {
    impl = std::move(other);
    construct(other);
  }

  void move_assign(implementation_type& impl,
                   stream_fiber_service& other_service,
                   implementation_type& other_impl) {
    //
  }

  /// Destroy a fiber implementation.
  void destroy(implementation_type& impl) { impl.reset(); }

  /// Open a stream socket.
  boost::system::error_code open(implementation_type& impl,
                                  const protocol_type& protocol,
                                  boost::system::error_code& ec)
  {
    return ec;
  }

  /// Determine whether the fiber is open.
  bool is_open(const implementation_type& impl) const
  {
    boost::recursive_mutex::scoped_lock lock_state(impl->state_mutex);
    return !impl->closed;
  }

  /// Close a fiber implementation.
  boost::system::error_code close(implementation_type& impl,
                                  boost::system::error_code& ec)
  {
    impl->p_fib_demux->close_fiber(impl);
    return ec;
  }
  
  /// Cancel all asynchronous operations associated with the stream fiber.
  boost::system::error_code cancel(implementation_type& impl,
                                   boost::system::error_code& ec)
  {
    impl->cancel_operations();
    return ec;
  }

  /// Bind the fiber to the specified local/remote endpoint.
  boost::system::error_code bind(implementation_type& impl,
                                  const endpoint_type& endpoint,
                                  boost::system::error_code& ec)
  {
    impl->p_fib_demux = &(endpoint.demux());
    impl->p_fib_demux->bind(endpoint.port(), impl, ec);

    return ec;
  }

  /// Start an asynchronous connect.
  template <typename ConnectHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ConnectHandler,
                                void(boost::system::error_code))
    async_connect(implementation_type& impl,
                  const endpoint_type& peer_endpoint,
                  BOOST_ASIO_MOVE_ARG(ConnectHandler) handler)
  {
    boost::asio::detail::async_result_init<ConnectHandler,
                                           void(boost::system::error_code)>
      init(BOOST_ASIO_MOVE_CAST(ConnectHandler)(handler));

    impl->connect_user_handler = init.handler;
    impl->p_fib_demux = &(peer_endpoint.demux());
    impl->p_fib_demux->async_connect(peer_endpoint.port(), impl);

    return init.result.get();
  }

  /// Get the local endpoint.
  endpoint_type local_endpoint(const implementation_type& impl,
                                boost::system::error_code& ec) const
  {
    return endpoint_type(*(impl->p_fib_demux), impl->id.local_port());
  }

  /// Get the remote endpoint.
  endpoint_type remote_endpoint(const implementation_type& impl,
                                boost::system::error_code& ec) const
  {
    return endpoint_type(*(impl->p_fib_demux), impl->id.remote_port());
  }

  /// (Deprecated: Use native_handle().) Get the native socket implementation.
  native_type native(implementation_type& impl)
  {
    return impl;
  }

  /// Get the native socket implementation.
  native_handle_type native_handle(implementation_type& impl)
  {
    return impl;
  }

  /// Start an asynchronous send.
  template <typename ConstBufferSequence, typename WriteHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler,
                                void(boost::system::error_code, std::size_t))
    async_send(implementation_type& impl,
               const ConstBufferSequence& buffers,
               socket_base::message_flags flags,
               BOOST_ASIO_MOVE_ARG(WriteHandler) handler)
  {
    boost::asio::detail::async_result_init<
      WriteHandler, void(boost::system::error_code, std::size_t)> init(
      BOOST_ASIO_MOVE_CAST(WriteHandler)(handler));

    if (!impl)
    {
      auto handler_to_post = [init]() mutable {
        init.handler(
          boost::system::error_code(::error::bad_file_descriptor,
                                    ::error::get_ssf_category()),
          0);
      };
      this->get_io_service().post(handler_to_post);
    }
    else
    {

      {
        boost::recursive_mutex::scoped_lock lock_state(impl->state_mutex);
        if (!impl->connected) {
          auto handler_to_post = [init]() mutable {
            init.handler(
              boost::system::error_code(::error::not_connected,
              ::error::get_ssf_category()),
              0);
          };
          this->get_io_service().post(handler_to_post);
          return init.result.get();
        }
      }

      // Call handler immediatly if buffer size at 0
      if (boost::asio::buffer_size(buffers) == 0)
      {
        auto handler_to_post = [init]() mutable {
          init.handler(
            boost::system::error_code(::error::success,
                                      ::error::get_ssf_category()),
            0);
        };
        this->get_io_service().post(handler_to_post);
      }
      else
      {
        // Service behaviour
        impl->p_fib_demux->async_send(buffers, impl->id, init.handler);
      }
    }

    return init.result.get();
  }

  /// Start an asynchronous receive.
  template <typename MutableBufferSequence, typename ReadHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
                                void(boost::system::error_code, std::size_t))
    async_receive(implementation_type& impl,
                  const MutableBufferSequence& buffers,
                  socket_base::message_flags flags,
                  BOOST_ASIO_MOVE_ARG(ReadHandler) handler)
  {
    boost::asio::detail::async_result_init<
      ReadHandler, void(boost::system::error_code, std::size_t)> init(
      BOOST_ASIO_MOVE_CAST(ReadHandler)(handler));

    {
      boost::recursive_mutex::scoped_lock lock_state(impl->state_mutex);
      if (!impl->connected) {
        auto handler_to_post = [init]() mutable {
          init.handler(
            boost::system::error_code(::error::not_connected,
            ::error::get_ssf_category()),
            0);
        };
        this->get_io_service().post(handler_to_post);
        return init.result.get();
      }
    }

    // Call handler immediatly if buffer size at 0
    if (boost::asio::buffer_size(buffers) == 0) {
      auto handler_to_post = [init]() mutable {
        init.handler(
          boost::system::error_code(::error::success,
                                    ::error::get_ssf_category()),
          0);
      };
      this->get_io_service().post(handler_to_post);
    }
    else
    {
      typedef detail::pending_read_operation<MutableBufferSequence, ReadHandler> op;
      typename op::ptr p = {
        boost::asio::detail::addressof(init.handler),
        boost_asio_handler_alloc_helpers::allocate(sizeof(op), init.handler), 0 };

      p.p = new (p.v) op(buffers, init.handler);

      {
        boost::recursive_mutex::scoped_lock lock(impl->read_op_queue_mutex);
        impl->read_op_queue.push(p.p);
      }
      p.v = p.p = 0;
      impl->r_queues_handler();
    }

    return init.result.get();
  }

  /// Disable sends or receives on the fiber.
  boost::system::error_code shutdown(implementation_type& impl,
                                      socket_base::shutdown_type what,
                                      boost::system::error_code& ec)
  {
    return ec;
  }

private:
  // Destroy all user-defined handler objects owned by the service.
  void shutdown_service()
  {
  }
};

} // namespace fiber
} // namespace asio
} // namespace boost

#endif  // SSF_COMMON_BOOST_ASIO_FIBER_STREAM_FIBER_SERVICE_HPP_
