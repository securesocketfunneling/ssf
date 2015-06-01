//
// fiber/fiber_acceptor_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2014-2015
//

#ifndef SSF_COMMON_BOOST_ASIO_FIBER_FIBER_ACCEPTOR_SERVICE_HPP
#define SSF_COMMON_BOOST_ASIO_FIBER_FIBER_ACCEPTOR_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif  // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/basic_socket.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/io_service.hpp>

#include "common/boost/fiber/basic_fiber_demux.hpp"
#include "common/boost/fiber/detail/fiber_id.hpp"
#include "common/boost/fiber/basic_endpoint.hpp"
#include "common/boost/fiber/detail/basic_fiber_impl.hpp"

namespace boost {
namespace asio {
namespace fiber {

/// Default service implementation for a fiber acceptor.
template <typename Protocol>
class fiber_acceptor_service
#if defined(GENERATING_DOCUMENTATION)
    : public boost::asio::io_service::service
#else
    : public boost::asio::detail::service_base<
          fiber_acceptor_service<Protocol> >
#endif
      {
 public:
  /// The protocol type.
  typedef Protocol protocol_type;

  /// The endpoint type.
  typedef typename Protocol::endpoint endpoint_type;

  /// Type of the demultiplexer.
  typedef typename Protocol::demux_type demux_type;

  typedef boost::asio::fiber::detail::basic_fiber_impl<
      typename Protocol::socket_type> implementation_deref_type;

  /// Implementation type
  typedef std::shared_ptr<implementation_deref_type> implementation_type;

/// (Deprecated: Use native_handle_type.) The native fiber type.
#if defined(GENERATING_DOCUMENTATION)
  typedef implementation_defined native_type;
#else
  typedef implementation_type native_type;
#endif

/// The native fiber type
#if defined(GENERATING_DOCUMENTATION)
  typedef implementation_defined native_handle_type;
#else
  typedef implementation_type native_handle_type;
#endif

  /// Construct a new fiber acceptor service for the specified io_service.
  explicit fiber_acceptor_service(boost::asio::io_service& io_service)
      : boost::asio::detail::service_base<fiber_acceptor_service<Protocol> >(
            io_service) {}

  /// Construct a new fiber acceptor implementation.
  void construct(implementation_type& impl) {
    impl = implementation_deref_type::create();
  }

  void move_construct(implementation_type& impl, implementation_type& other) {
    impl = std::move(other);
    construct(other);
  }

  void move_assign(implementation_type& impl,
                   fiber_acceptor_service& other_service,
                   implementation_type& other_impl) {
    //
  }

  /// Destroy a fiber acceptor implementation.
  void destroy(implementation_type& impl) {}

  /// Open a new fiber acceptor implementation.
  boost::system::error_code open(implementation_type& impl,
                                 const protocol_type& protocol,
                                 boost::system::error_code& ec) {
    return ec;
  }

  /// Determine whether the acceptor is open.
  bool is_open(const implementation_type& impl) const {
    boost::recursive_mutex::scoped_lock lock_state(impl->state_mutex);
    return !impl->closed;
  }

  /// Cancel all asynchronous operations associated with the acceptor.
  boost::system::error_code cancel(implementation_type& impl,
                                   boost::system::error_code& ec) {
    impl->cancel_operations();
    return ec;
  }

  /// Bind the fiber acceptor to the specified local endpoint.
  boost::system::error_code bind(implementation_type& impl,
                                 const endpoint_type& endpoint,
                                 boost::system::error_code& ec) {
    impl->p_fib_demux = &(endpoint.demux());
    impl->p_fib_demux->bind(endpoint.port(), impl, ec);

    return ec;
  }

  /// Place the fiber acceptor into the state where it will listen for new
  /// connections.
  boost::system::error_code listen(implementation_type& impl, int backlog,
                                   boost::system::error_code& ec) {
    impl->p_fib_demux->listen(impl->id.local_port(), ec);
    return ec;
  }

  /// Close a fiber acceptor implementation.
  boost::system::error_code close(implementation_type& impl,
                                  boost::system::error_code& ec) {
    if (!impl->p_fib_demux) {
      return ec;
    }

    impl->p_fib_demux->close_fiber(impl);
    return ec;
  }

  /// (Deprecated: Use native_handle().) Get the native acceptor implementation.
  native_type native(implementation_type& impl) { return impl; }

  /// Get the native acceptor implementation.
  native_handle_type native_handle(implementation_type& impl) { return impl; }

  /// Get the local endpoint.
  endpoint_type local_endpoint(const implementation_type& impl,
                               boost::system::error_code& ec) const {
    return endpoint_type(*(impl->p_fib_demux), impl->id.local_port());
  }

  /// Start an asynchronous accept.
  template <typename Protocol1, typename SocketService, typename AcceptHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(AcceptHandler, void(boost::system::error_code))
      async_accept(implementation_type& impl,
                   basic_socket<Protocol1, SocketService>& peer,
                   endpoint_type* peer_endpoint,
                   BOOST_ASIO_MOVE_ARG(AcceptHandler) handler,
                   typename enable_if<
                       is_convertible<Protocol, Protocol1>::value>::type* = 0) {
    boost::asio::detail::async_result_init<AcceptHandler,
                                           void(boost::system::error_code)>
                                           init(BOOST_ASIO_MOVE_CAST(AcceptHandler)(handler));
    
    {
      boost::recursive_mutex::scoped_lock lock_state(impl->state_mutex);
      if (impl->closed) {
        auto handler_to_post = [init]() mutable {
          init.handler(
            boost::system::error_code(ssf::error::not_connected,
            ssf::error::get_ssf_category()));
        };
        this->get_io_service().post(handler_to_post);

        return init.result.get();
      }
    }

    boost::system::error_code ec;

    auto fiber_impl = peer.native_handle();
    fiber_impl->p_fib_demux = impl->p_fib_demux;
    fiber_impl->id.set_local_port(impl->id.local_port());

    BOOST_LOG_TRIVIAL(debug) << "fiber acceptor: local port set " << impl->id.local_port();

    typedef detail::pending_accept_operation<
        AcceptHandler, typename Protocol::socket_type> op;
    typename op::ptr p = {
        boost::asio::detail::addressof(init.handler),
        boost_asio_handler_alloc_helpers::allocate(sizeof(op), init.handler),
        0};

    p.p = new (p.v)
        op(peer.native_handle(), &(peer.native_handle()->id), init.handler);
    {
      boost::recursive_mutex::scoped_lock lock(impl->accept_op_queue_mutex);
      impl->accept_op_queue.push(p.p);
    }
    p.v = p.p = 0;
    impl->a_queues_handler();

    return init.result.get();
  }

 private:
  // Destroy all user-defined handler objects owned by the service.
  void shutdown_service() {}
};

}  // namespace fiber
}  // namespace asio
}  // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif  // SSF_COMMON_BOOST_ASIO_FIBER_FIBER_ACCEPTOR_SERVICE_HPP
