#ifndef SSF_LAYER_INTERFACE_LAYER_BASIC_INTERFACE_SERVICE_H_
#define SSF_LAYER_INTERFACE_LAYER_BASIC_INTERFACE_SERVICE_H_

#include <functional>
#include <memory>

#include <boost/asio/async_result.hpp>
#include <boost/asio/handler_type.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/socket_base.hpp>

#include <boost/system/error_code.hpp>

#include "ssf/error/error.h"
#include "ssf/io/composed_op.h"

#include "ssf/layer/interface_layer/generic_interface_socket.h"
#include "ssf/layer/interface_layer/specific_interface_socket.h"

namespace ssf {
namespace layer {
namespace interface_layer {

#include <boost/asio/detail/push_options.hpp>

template <class Protocol, class NextLayerProtocol>
class basic_Interface_service
    : public boost::asio::detail::service_base<
          basic_Interface_service<Protocol, NextLayerProtocol>> {
 public:
  typedef Protocol protocol_type;
  typedef NextLayerProtocol raw_next_layer_protocol;
  typedef typename protocol_type::endpoint_context_type endpoint_context_type;
  typedef typename raw_next_layer_protocol::socket next_socket_type;
  typedef typename raw_next_layer_protocol::acceptor next_acceptor_type;
  typedef std::shared_ptr<next_socket_type> p_next_socket_type;
  typedef std::shared_ptr<next_acceptor_type> p_next_acceptor_type;

  struct implementation_type {
    endpoint_context_type interface_name;
    p_next_socket_type p_next_socket;
    p_next_acceptor_type p_next_acceptor;
  };

  typedef implementation_type& native_handle_type;
  typedef native_handle_type native_type;

 public:
  explicit basic_Interface_service(boost::asio::io_service& io_service)
      : boost::asio::detail::service_base<basic_Interface_service>(io_service) {
  }

  virtual ~basic_Interface_service() {}

  void construct(implementation_type& impl) {}

  void destroy(implementation_type& impl) {
    protocol_type::get_interface_manager().Erase(impl.interface_name);

    impl.interface_name.clear();
    impl.p_next_socket.reset();
  }

  void move_construct(implementation_type& impl, implementation_type& other) {
    impl = std::move(other);
  }

  void move_assign(implementation_type& impl, implementation_type& other) {
    impl = std::move(other);
  }

  /// Create a socket of the next layer type,
  /// async connect it to its given endpoint and
  /// register it as the interface name
  template <class ConnectHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ConnectHandler, void(boost::system::error_code))
  async_connect(
      implementation_type& impl, endpoint_context_type interface_name,
      const typename raw_next_layer_protocol::endpoint& next_remote_endpoint,
      ConnectHandler&& handler) {
    boost::asio::detail::async_result_init<ConnectHandler,
                                           void(boost::system::error_code)>
        init(std::forward<ConnectHandler>(handler));

    impl.interface_name = interface_name;
    bool interface_already_exists = true;
    if (!protocol_type::get_interface_manager().Find(interface_name)) {
      interface_already_exists = false;
      impl.p_next_socket =
          std::make_shared<typename raw_next_layer_protocol::socket>(
              this->get_io_service());
    }

    if (impl.p_next_socket->is_open()) {
      this->get_io_service().post(std::bind(
          init.handler,
          boost::system::error_code(ssf::error::device_or_resource_busy,
                                    ssf::error::get_ssf_category())));
      return init.result.get();
    }

    auto connected_lambda = [this, init, &impl, interface_already_exists](
        const boost::system::error_code& ec) {
      auto updated_ec = ec;
      if (!ec) {
        if (!interface_already_exists) {
          // encapsulate next layer socket in specific interface socket
          auto p_specific_socket =
              specific_interface_socket<Protocol, NextLayerProtocol>::Create(
                  impl.p_next_socket);

          auto endpoint_inserted =
              protocol_type::get_interface_manager().Emplace(
                  impl.interface_name, p_specific_socket);

          if (!endpoint_inserted) {
            updated_ec.assign(ssf::error::device_or_resource_busy,
                              ssf::error::get_ssf_category());
          }
        } else {
          auto p_socket_optional =
              protocol_type::get_interface_manager().Find(impl.interface_name);
          if (p_socket_optional) {
            boost::system::error_code ec;
            (*p_socket_optional)->connect(ec);
          }
        }
      } else {
        boost::system::error_code close_ec;
        impl.p_next_socket->close(close_ec);
      }

      init.handler(updated_ec);
    };

    return impl.p_next_socket->async_connect(
        next_remote_endpoint,
        io::ComposedOp<
            decltype(connected_lambda),
            typename boost::asio::handler_type<
                ConnectHandler, void(boost::system::error_code)>::type>(
            std::move(connected_lambda), init.handler));
  }

  /// Create an acceptor of the next layer type,
  /// make it listening to its given endpoint, async accept connection and
  /// register the first connection (next layer socket) as the interface name
  template <class AcceptHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(AcceptHandler, void(boost::system::error_code))
  async_accept(
      implementation_type& impl, const endpoint_context_type& interface_name,
      const typename raw_next_layer_protocol::endpoint& next_local_endpoint,
      AcceptHandler&& handler) {
    boost::asio::detail::async_result_init<AcceptHandler,
                                           void(boost::system::error_code)>
        init(std::forward<AcceptHandler>(handler));

    impl.interface_name = interface_name;
    bool interface_already_exists = true;

    if (!impl.p_next_socket) {
      interface_already_exists = false;
      impl.p_next_socket =
          std::make_shared<typename raw_next_layer_protocol::socket>(
              this->get_io_service());
    }

    if (impl.p_next_socket->is_open()) {
      this->get_io_service().post(std::bind(
          init.handler,
          boost::system::error_code(ssf::error::device_or_resource_busy,
                                    ssf::error::get_ssf_category())));
      return init.result.get();
    }

    if (!impl.p_next_acceptor) {
      impl.p_next_acceptor =
          std::make_shared<typename raw_next_layer_protocol::acceptor>(
              this->get_io_service());
      boost::system::error_code ec;
      impl.p_next_acceptor->open(next_local_endpoint.protocol());
      impl.p_next_acceptor->set_option(
          boost::asio::socket_base::reuse_address(true), ec);
      impl.p_next_acceptor->bind(next_local_endpoint, ec);

      if (ec) {
        this->get_io_service().post(std::bind(init.handler, ec));
        return init.result.get();
      }

      impl.p_next_acceptor->listen();
    }

    auto p_next_remote_endpoint =
        std::make_shared<typename raw_next_layer_protocol::endpoint>();
    auto p_next_acceptor = impl.p_next_acceptor;
    auto p_next_socket = impl.p_next_socket;
    auto accepted_lambda = [this, &impl, p_next_remote_endpoint, init,
                            p_next_acceptor, p_next_socket,
                            interface_already_exists](
        const boost::system::error_code& ec) mutable {
      auto updated_ec = ec;
      if (!ec) {
        if (!interface_already_exists) {
          // encapsulate next layer socket in specific interface socket
          auto p_specific_socket =
              specific_interface_socket<Protocol, NextLayerProtocol>::Create(
                  impl.p_next_socket);
          auto endpoint_inserted =
              protocol_type::get_interface_manager().Emplace(
                  impl.interface_name, p_specific_socket);

          if (!endpoint_inserted) {
            updated_ec.assign(ssf::error::device_or_resource_busy,
                              ssf::error::get_ssf_category());
          }
        } else {
          auto p_socket_optional =
              protocol_type::get_interface_manager().Find(impl.interface_name);
          if (p_socket_optional) {
            boost::system::error_code ec;
            (*p_socket_optional)->connect(ec);
          }
        }
      } else {
        boost::system::error_code close_ec;
        impl.p_next_socket->close(close_ec);
      }

      if (p_next_acceptor) {
        boost::system::error_code other_ec;
        p_next_acceptor->close(other_ec);
        p_next_acceptor.reset();
        impl.p_next_acceptor.reset();
      }

      init.handler(updated_ec);
    };

    return impl.p_next_acceptor->async_accept(
        *impl.p_next_socket, *p_next_remote_endpoint,
        io::ComposedOp<
            decltype(accepted_lambda),
            typename boost::asio::handler_type<
                AcceptHandler, void(boost::system::error_code)>::type>(
            std::move(accepted_lambda), init.handler));
  }

  boost::system::error_code close(implementation_type& impl,
                                  boost::system::error_code& ec) {
    protocol_type::get_interface_manager().Erase(impl.interface_name);

    if (impl.p_next_acceptor) {
      impl.p_next_acceptor->close(ec);
    }

    if (!impl.p_next_socket) {
      return ec;
    }

    impl.p_next_socket->shutdown(boost::asio::socket_base::shutdown_both, ec);
    return impl.p_next_socket->close(ec);
  }

  bool is_open(implementation_type& impl) const {
    if (impl.p_next_acceptor) {
      return true;
    }
    if (!impl.p_next_socket) {
      return false;
    }
    return impl.p_next_socket->is_open();
  }

 private:
  void shutdown_service() {}
};

#include <boost/asio/detail/pop_options.hpp>

}  // interface_layer
}  // layer
}  // ssf

#endif  // SSF_LAYER_INTERFACE_LAYER_BASIC_INTERFACE_SERVICE_H_
