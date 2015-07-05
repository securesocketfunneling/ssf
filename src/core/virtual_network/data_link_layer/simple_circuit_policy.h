#ifndef SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_SIMPLE_CIRCUIT_POLICY_H_
#define SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_SIMPLE_CIRCUIT_POLICY_H_

#include <cstdint>

#include <memory>

#include <boost/system/error_code.hpp>

#include "common/network/object_io_helpers.h"

#include "core/virtual_network/data_link_layer/circuit_helpers.h"

namespace virtual_network {
namespace data_link_layer {

template <class Protocol>
class CircuitPolicy {
public:
  enum { client = 0, server = 1 };

private:
  typedef typename Protocol::next_layer_protocol::socket next_socket_type;
  typedef typename Protocol::endpoint endpoint_type;
  typedef typename Protocol::resolver resolver_type;

public:
  static void InitConnection(next_socket_type &next_socket,
                             endpoint_type *p_remote_endpoint, uint8_t type,
                             boost::system::error_code &ec) {
    return;
  }

  template <class Handler>
  static void AsyncInitConnection(next_socket_type &next_socket,
                                  endpoint_type *p_remote_endpoint,
                                  uint8_t type, Handler handler) {
    if (type == client) {
      AsyncInitConnectionClient(next_socket, p_remote_endpoint, handler);
    } else {
      AsyncInitConnectionServer(next_socket, p_remote_endpoint, handler);
    }
  }

  template <class Handler>
  static void AsyncValidateConnection(next_socket_type &next_socket,
                                      endpoint_type *p_remote_endpoint,
                                      uint32_t ec_value, Handler handler) {
    ssf::SendBase<uint32_t>(next_socket, ec_value, handler);
  }

private:
  /// Send protocol data (circuit nodes), wait validation,
  ///   execute given handler
  template <class Handler>
  static void AsyncInitConnectionClient(next_socket_type &next_socket,
                                        endpoint_type *p_remote_endpoint,
                                        Handler handler) {
    auto string_sent_lambda = [&next_socket, handler](
        const boost::system::error_code &ec) mutable {
      if (!ec) {
        auto p_value = std::make_shared<uint32_t>(0);

        auto ec_value_received_lambda = [handler, p_value](
            const boost::system::error_code &ec) mutable {
          if (!ec) {
            handler(boost::system::error_code(
                *p_value, boost::system::system_category()));
          } else {
            handler(ec);
          }
        };

        ssf::ReceiveBase<uint32_t>(next_socket, p_value.get(),
                                   ec_value_received_lambda);
      } else {
        handler(ec);
      }
    };

    ssf::SendString(next_socket,
                    p_remote_endpoint->endpoint_context().forward_blocks,
                    string_sent_lambda);
  }

  /// Read the protocol data, extract parameters stack and populate
  /// p_remote_endpoint with received data, execute given handler
  template <class Handler>
  static void AsyncInitConnectionServer(next_socket_type &next_socket,
                                        endpoint_type *p_received_endpoint,
                                        Handler handler) {
    auto p_string = std::make_shared<std::string>();

    auto string_received_lambda =
      [&next_socket, handler, p_string, p_received_endpoint]
    (const boost::system::error_code &ec) mutable {

      if (ec) {
        handler(ec);
        return;
      }

      auto stack = detail::make_parameter_stack(*p_string);

      if (stack.size() == 1) {
        p_received_endpoint->endpoint_context() = detail::make_circuit_context(
            next_socket.get_io_service(), stack.front());
        handler(ec);
        return;
      }

      resolver_type resolver(next_socket.get_io_service());
      boost::system::error_code resolve_ec;
      auto endpoint_it = resolver.resolve(stack, resolve_ec);

      if (resolve_ec) {
        handler(resolve_ec);
        return;
      }

      *p_received_endpoint = *endpoint_it;

      handler(ec);
      return;
    };

    ssf::ReceiveString(next_socket, p_string.get(), string_received_lambda);
  }
};

} // data_link_layer
} // virtual_network

#endif // SSF_CORE_VIRTUAL_NETWORK_DATA_LINK_SIMPLE_CIRCUIT_POLICY_H_