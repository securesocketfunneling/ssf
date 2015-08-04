#ifndef SSF_CORE_VIRTUAL_NETWORK_BASIC_EMPTY_DATAGRAM_H_
#define SSF_CORE_VIRTUAL_NETWORK_BASIC_EMPTY_DATAGRAM_H_

#include <map>
#include <memory>
#include <string>

#include <boost/asio/async_result.hpp>
#include <boost/asio/basic_datagram_socket.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/type_traits.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/socket_base.hpp>

#include <boost/system/error_code.hpp>

#include "common/error/error.h"
#include "common/io/composed_op.h"

#include "core/virtual_network/basic_impl.h"
#include "core/virtual_network/basic_resolver.h"
#include "core/virtual_network/basic_endpoint.h"

#include "core/virtual_network/protocol_attributes.h"
#include "core/virtual_network/parameters.h"

#include "core/virtual_network/connect_op.h"
#include "core/virtual_network/receive_from_op.h"

namespace virtual_network {

template <class Protocol>
class VirtualEmptyDatagramSocket_service;

template <class NextLayer>
class VirtualEmptyDatagramProtocol {
 public:
  enum {
    id = 3,
    overhead = 0,
    facilities = virtual_network::facilities::datagram,
    mtu = NextLayer::mtu - overhead
  };
  enum { endpoint_stack_size = NextLayer::endpoint_stack_size };

  typedef NextLayer next_layer_protocol;
  typedef int socket_context;
  typedef int endpoint_context_type;
  using next_endpoint_type = typename next_layer_protocol::endpoint;

  typedef basic_VirtualLink_endpoint<VirtualEmptyDatagramProtocol> endpoint;
  typedef basic_VirtualLink_resolver<VirtualEmptyDatagramProtocol> resolver;
  typedef boost::asio::basic_datagram_socket<
      VirtualEmptyDatagramProtocol,
      VirtualEmptyDatagramSocket_service<VirtualEmptyDatagramProtocol>> socket;

private:
 using query = typename resolver::query;

 public:
  static endpoint make_endpoint(boost::asio::io_service& io_service,
                                typename query::const_iterator parameters_it,
                                uint32_t, boost::system::error_code& ec) {
    return endpoint(next_layer_protocol::make_endpoint(
        io_service, parameters_it, id, ec));
  }

  static std::string get_address(const endpoint& endpoint) {
    return endpoint.next_layer_endpoint().address().to_string();  
  }
};

#include <boost/asio/detail/push_options.hpp>

template <class Protocol>
class VirtualEmptyDatagramSocket_service
    : public boost::asio::detail::service_base<
          VirtualEmptyDatagramSocket_service<Protocol>> {
 public:
  /// The protocol type.
  typedef Protocol protocol_type;
  /// The endpoint type.
  typedef typename protocol_type::endpoint endpoint_type;

  typedef basic_socket_impl<protocol_type> implementation_type;
  typedef implementation_type& native_handle_type;
  typedef native_handle_type native_type;

 private:
  typedef typename protocol_type::next_layer_protocol::socket next_socket_type;

 public:
  explicit VirtualEmptyDatagramSocket_service(
      boost::asio::io_service& io_service)
      : boost::asio::detail::service_base<VirtualEmptyDatagramSocket_service>(
            io_service) {}

  virtual ~VirtualEmptyDatagramSocket_service() {}

  void construct(implementation_type& impl) {
    impl.p_next_layer_socket =
        std::make_shared<next_socket_type>(this->get_io_service());
  }

  void destroy(implementation_type& impl) {
    impl.p_local_endpoint.reset();
    impl.p_remote_endpoint.reset();
    impl.p_next_layer_socket.reset();
  }

  void move_construct(implementation_type& impl, implementation_type& other) {
    impl = std::move(other);
  }

  void move_assign(implementation_type& impl, implementation_type& other) {
    impl = std::move(other);
  }

  boost::system::error_code open(implementation_type& impl,
                                 const protocol_type& protocol,
                                 boost::system::error_code& ec) {
    return impl.p_next_layer_socket->open(typename protocol_type::next_layer_protocol(),
                                          ec);
  }

  boost::system::error_code assign(implementation_type& impl,
                                   const protocol_type& protocol,
                                   const native_handle_type& native_socket,
                                   boost::system::error_code& ec) {
    impl = native_socket;
    return ec;
  }

  bool is_open(const implementation_type& impl) const {
    return impl.p_next_layer_socket->is_open();
  }

  endpoint_type remote_endpoint(const implementation_type& impl,
                                boost::system::error_code& ec) const {
    if (impl.p_remote_endpoint) {
      ec.assign(ssf::error::success, ssf::error::get_ssf_category());
      return *impl.p_remote_endpoint;
    } else {
      ec.assign(ssf::error::no_link, ssf::error::get_ssf_category());
      return endpoint_type();
    }
  }

  endpoint_type local_endpoint(const implementation_type& impl,
                               boost::system::error_code& ec) const {
    if (impl.p_local_endpoint) {
      ec.assign(ssf::error::success, ssf::error::get_ssf_category());
      return *impl.p_local_endpoint;
    } else {
      ec.assign(ssf::error::no_link, ssf::error::get_ssf_category());
      return endpoint_type();
    }
  }

  boost::system::error_code close(implementation_type& impl,
                                  boost::system::error_code& ec) {
    return impl.p_next_layer_socket->close(ec);
  }

  native_type native(implementation_type& impl) { return impl; }

  native_handle_type native_handle(implementation_type& impl) { return impl; }

  // boost::system::error_code cancel(implementation_type& impl,
  //                                 boost::system::error_code& ec) {
  //  return impl.p_next_layer_socket->cancel(ec);
  //}

  bool at_mark(const implementation_type& impl,
               boost::system::error_code& ec) const {
    return impl.p_next_layer_socket->at_mark(ec);
  }

  std::size_t available(const implementation_type& impl,
                        boost::system::error_code& ec) const {
    if (!impl.p_next_layer_socket) {
      ec.assign(ssf::error::bad_file_descriptor,
                ssf::error::get_ssf_category());
      return 0;
    }

    return impl.p_next_layer_socket->available(ec);
  }

  boost::system::error_code bind(implementation_type& impl,
                                 const endpoint_type& endpoint,
                                 boost::system::error_code& ec) {
    impl.p_local_endpoint = std::make_shared<endpoint_type>(endpoint);
    return impl.p_next_layer_socket->bind(endpoint.next_layer_endpoint(), ec);
  }

  boost::system::error_code connect(implementation_type& impl,
                                    const endpoint_type& peer_endpoint,
                                    boost::system::error_code& ec) {
    impl.p_remote_endpoint = std::make_shared<endpoint_type>(peer_endpoint);

    impl.p_next_layer_socket->connect(peer_endpoint.next_layer_endpoint(), ec);

    if (!ec) {
      impl.p_local_endpoint = std::make_shared<endpoint_type>(
          impl.p_next_layer_socket->local_endpoint(ec));
    }

    return ec;
  }

  template <typename ConnectHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ConnectHandler, void(boost::system::error_code))
      async_connect(implementation_type& impl,
                    const endpoint_type& peer_endpoint,
                    ConnectHandler&& handler) {
    boost::asio::detail::async_result_init<ConnectHandler,
                                           void(boost::system::error_code)>
        init(std::forward<ConnectHandler>(handler));

    impl.p_remote_endpoint = std::make_shared<endpoint_type>(peer_endpoint);
    impl.p_local_endpoint = std::make_shared<endpoint_type>(0);

    detail::ConnectOp<
        protocol_type, next_socket_type, endpoint_type,
        typename boost::asio::handler_type<
            ConnectHandler,
            void(boost::system::error_code)>::type>(*impl.p_next_layer_socket,
                                                    impl.p_local_endpoint.get(),
                                                    peer_endpoint,
                                                    init.handler)();

    return init.result.get();
  }

  template <typename ConstBufferSequence, typename WriteHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler,
                                void(boost::system::error_code, std::size_t))
      async_send(implementation_type& impl, const ConstBufferSequence& buffers,
                 boost::asio::socket_base::message_flags flags,
                 WriteHandler&& handler) {
    return impl.p_next_layer_socket->async_send(
        buffers, std::forward<WriteHandler>(handler));
  }

  template <typename ConstBufferSequence, typename WriteHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler,
                                void(boost::system::error_code, std::size_t))
      async_send_to(implementation_type& impl,
                    const ConstBufferSequence& buffers,
                    const endpoint_type& destination,
                    boost::asio::socket_base::message_flags flags,
                    WriteHandler&& handler) {
    return impl.p_next_layer_socket->async_send_to(
        buffers, destination.next_layer_endpoint(),
        std::forward<WriteHandler>(handler));
  }

  template <typename MutableBufferSequence, typename ReadHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
                                void(boost::system::error_code, std::size_t))
      async_receive(implementation_type& impl,
                    const MutableBufferSequence& buffers,
                    boost::asio::socket_base::message_flags flags,
                    ReadHandler&& handler) {
    return impl.p_next_layer_socket->async_receive(
        buffers, std::forward<ReadHandler>(handler));
  }

  template <typename MutableBufferSequence, typename ReadHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
                                void(boost::system::error_code, std::size_t))
      async_receive_from(implementation_type& impl,
                         const MutableBufferSequence& buffers,
                         endpoint_type& sender_endpoint,
                         boost::asio::socket_base::message_flags flags,
                         ReadHandler&& handler) {
    boost::asio::detail::async_result_init<
        ReadHandler, void(boost::system::error_code, std::size_t)>
        init(std::forward<ReadHandler>(handler));

    detail::ReceiveFromOp<
        protocol_type, next_socket_type, endpoint_type, MutableBufferSequence,
        typename boost::asio::
            handler_type<ReadHandler, void(boost::system::error_code)>::type>(
        *impl.p_next_layer_socket, &sender_endpoint, buffers, flags,
        init.handler)();

    return init.result.get();
  }

  boost::system::error_code shutdown(
      implementation_type& impl, boost::asio::socket_base::shutdown_type what,
      boost::system::error_code& ec) {
    impl.p_next_layer_socket->shutdown(what, ec);

    return ec;
  }

 private:
  void shutdown_service() {}
};

#include <boost/asio/detail/pop_options.hpp>

template <class NextLayer>
using VirtualEmptyDatagram_endpoint =
    typename VirtualEmptyDatagramProtocol<NextLayer>::endpoint;
template <class NextLayer>
using VirtualEmptyDatagram_socket =
    typename VirtualEmptyDatagramProtocol<NextLayer>::socket;
template <class NextLayer>
using VirtualEmptyDatagram_resolver =
    typename VirtualEmptyDatagramProtocol<NextLayer>::resolver;

}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_BASIC_EMPTY_DATAGRAM_H_
