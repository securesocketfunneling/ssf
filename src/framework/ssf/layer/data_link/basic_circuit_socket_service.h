#ifndef SSF_LAYER_DATA_LINK_BASIC_CIRCUIT_SOCKET_SERVICE_H_
#define SSF_LAYER_DATA_LINK_BASIC_CIRCUIT_SOCKET_SERVICE_H_

#include <memory>

#include <boost/system/error_code.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/async_result.hpp>

#include "ssf/io/handler_helpers.h"
#include "ssf/error/error.h"

#include "ssf/layer/basic_impl.h"

#include "ssf/layer/data_link/circuit_helpers.h"
#include "ssf/layer/data_link/circuit_op.h"

namespace ssf {
namespace layer {
namespace data_link {

#include <boost/asio/detail/push_options.hpp>

template <class Protocol>
class basic_CircuitSocket_service
    : public boost::asio::detail::service_base<
          basic_CircuitSocket_service<Protocol>> {
 public:
  /// The protocol type.
  typedef Protocol protocol_type;
  /// The endpoint type.
  typedef typename protocol_type::endpoint endpoint_type;
  typedef typename protocol_type::resolver resolver_type;

  typedef basic_socket_impl<protocol_type> implementation_type;
  typedef implementation_type& native_handle_type;
  typedef native_handle_type native_type;

 private:
  typedef typename protocol_type::circuit_policy circuit_policy;
  typedef typename protocol_type::next_layer_protocol::socket next_socket_type;
  typedef std::shared_ptr<next_socket_type> p_next_socket_type;
  typedef std::shared_ptr<endpoint_type> p_endpoint_type;

 public:
  explicit basic_CircuitSocket_service(
      boost::asio::io_service& io_service)
      : boost::asio::detail::service_base<
            basic_CircuitSocket_service>(io_service) {}

  virtual ~basic_CircuitSocket_service() {}

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
    if (!impl.p_next_layer_socket) {
      ec.assign(ssf::error::bad_file_descriptor,
                ssf::error::get_ssf_category());
      return ec;
    }

    return impl.p_next_layer_socket->open(typename protocol_type::next_layer_protocol(),
                                          ec);
  }

  boost::system::error_code assign(implementation_type& impl,
                                   const protocol_type& protocol,
                                   native_handle_type& native_socket,
                                   boost::system::error_code& ec) {
    impl = native_socket;

    return ec;
  }

  bool is_open(const implementation_type& impl) const {
    if (!impl.p_next_layer_socket) {
      return false;
    }

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
    if (!impl.p_next_layer_socket) {
      ec.assign(ssf::error::bad_file_descriptor,
                ssf::error::get_ssf_category());
      return ec;
    }

    return impl.p_next_layer_socket->close(ec);
  }

  native_type native(implementation_type& impl) { return impl; }

  native_handle_type native_handle(implementation_type& impl) { return impl; }

  bool at_mark(const implementation_type& impl,
               boost::system::error_code& ec) const {
    if (!impl.p_next_layer_socket) {
      ec.assign(ssf::error::bad_file_descriptor,
                ssf::error::get_ssf_category());
      return false;
    }
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

  boost::system::error_code cancel(implementation_type& impl,
                                   boost::system::error_code& ec) {
    if (!impl.p_next_layer_socket) {
      ec.assign(ssf::error::bad_file_descriptor,
                ssf::error::get_ssf_category());
      return ec;
    }

    return impl.p_next_layer_socket->cancel(ec);
  }

  /// Nothing to bind for this layer, bind the next layer with its endpoint
  boost::system::error_code bind(implementation_type& impl,
                                 const endpoint_type& endpoint,
                                 boost::system::error_code& ec) {
    impl.p_local_endpoint = std::make_shared<endpoint_type>(endpoint);
    return impl.p_next_layer_socket->bind(endpoint.next_layer_endpoint(), ec);
  }

  /// Connect next layer to its endpoint
  /// Connect the circuit
  boost::system::error_code connect(implementation_type& impl,
                                    const endpoint_type& peer_endpoint,
                                    boost::system::error_code& ec) {
    impl.p_remote_endpoint = std::make_shared<endpoint_type>(peer_endpoint);

    impl.p_next_layer_socket->connect(peer_endpoint.next_layer_endpoint(), ec);

    impl.p_local_endpoint = std::make_shared<endpoint_type>();
    impl.p_local_endpoint->endpoint_context().id = detail::get_local_id();
    auto& next_layer_endpoint = impl.p_local_endpoint->next_layer_endpoint();
    next_layer_endpoint = impl.p_next_layer_socket->local_endpoint();
    impl.p_local_endpoint->set();

    if (ec) {
      return ec;
    }

    circuit_policy::InitConnection(*impl.p_next_layer_socket,
                                   impl.p_remote_endpoint.get(),
                                   circuit_policy::client, ec);

    return ec;
  }

  /// Async connect next layer to its endpoint
  /// Then async connect the circuit with the provided endpoint
  /// Then async execute handler
  template <typename ConnectHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ConnectHandler, void(boost::system::error_code))
      async_connect(implementation_type& impl,
                    const endpoint_type& peer_endpoint,
                    ConnectHandler&& handler) {
    boost::asio::detail::async_result_init<
        ConnectHandler, void(boost::system::error_code)>
        init(std::forward<ConnectHandler>(handler));

    impl.p_remote_endpoint = std::make_shared<endpoint_type>(peer_endpoint);
    impl.p_local_endpoint = std::make_shared<endpoint_type>();
    impl.p_local_endpoint->endpoint_context().id = detail::get_local_id();

    detail::CircuitConnectOp<
        protocol_type, next_socket_type, endpoint_type,
        typename boost::asio::handler_type<ConnectHandler,
                                           void(boost::system::error_code)>::
            type>(*impl.p_next_layer_socket, impl.p_local_endpoint.get(),
                  impl.p_remote_endpoint.get(), init.handler)();

    return init.result.get();
  }

  template <typename ConstBufferSequence>
  std::size_t send(implementation_type& impl,
                   const ConstBufferSequence& buffers,
                   boost::asio::socket_base::message_flags flags,
                   boost::system::error_code& ec) {
    ec.assign(ssf::error::function_not_supported,
              ssf::error::get_ssf_category());
    return 0;
  }

  template <typename ConstBufferSequence, typename WriteHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler,
                                void(boost::system::error_code, std::size_t))
      async_send(implementation_type& impl, const ConstBufferSequence& buffers,
                 boost::asio::socket_base::message_flags flags,
                 WriteHandler&& handler) {
    boost::asio::detail::async_result_init<
        WriteHandler, void(boost::system::error_code, std::size_t)>
        init(std::forward<WriteHandler>(handler));

    impl.p_next_layer_socket->async_send(buffers, init.handler);

    return init.result.get();
  }

  template <typename MutableBufferSequence>
  std::size_t receive(implementation_type& impl,
                      const MutableBufferSequence& buffers,
                      boost::asio::socket_base::message_flags flags,
                      boost::system::error_code& ec) {
    ec.assign(ssf::error::function_not_supported,
              ssf::error::get_ssf_category());
    return 0;
  }

  template <typename MutableBufferSequence, typename ReadHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
                                void(boost::system::error_code, std::size_t))
      async_receive(implementation_type& impl,
                    const MutableBufferSequence& buffers,
                    boost::asio::socket_base::message_flags flags,
                    ReadHandler&& handler) {
    boost::asio::detail::async_result_init<
        ReadHandler, void(boost::system::error_code, std::size_t)>
        init(std::forward<ReadHandler>(handler));

    impl.p_next_layer_socket->async_receive(buffers, init.handler);

    return init.result.get();
  }

  boost::system::error_code shutdown(
      implementation_type& impl, boost::asio::socket_base::shutdown_type what,
      boost::system::error_code& ec) {
    return impl.p_next_layer_socket->shutdown(what, ec);
  }

 private:
  void shutdown_service() {}
};

#include <boost/asio/detail/pop_options.hpp>

}  // data_link
}  // layer
}  // ssf

#endif  // SSF_LAYER_DATA_LINK_BASIC_CIRCUIT_SOCKET_SERVICE_H_
