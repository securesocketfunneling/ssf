#ifndef SSF_LAYER_BASIC_IMPL_H_
#define SSF_LAYER_BASIC_IMPL_H_

#include <memory>

namespace ssf {
namespace layer {

template <class Context, class Endpoint, class NextSocket>
struct basic_socket_impl_ex {
 private:
  typedef std::shared_ptr<Context> socket_context;
  typedef std::shared_ptr<Endpoint> endpoint_type;
  typedef std::shared_ptr<NextSocket> next_layer_socket_type;

 public:
  basic_socket_impl_ex()
      : p_socket_context(nullptr),
        p_local_endpoint(nullptr),
        p_remote_endpoint(nullptr),
        p_next_layer_socket(nullptr) {}

  basic_socket_impl_ex(basic_socket_impl_ex&& other)
      : p_socket_context(std::move(other.p_socket_context)),
        p_local_endpoint(std::move(other.p_local_endpoint)),
        p_remote_endpoint(std::move(other.p_remote_endpoint)),
        p_next_layer_socket(std::move(other.p_next_layer_socket)) {}

  basic_socket_impl_ex& operator=(basic_socket_impl_ex&& other) {
    p_socket_context = std::move(other.p_socket_context);
    p_local_endpoint = std::move(other.p_local_endpoint);
    p_remote_endpoint = std::move(other.p_remote_endpoint);
    p_next_layer_socket = std::move(other.p_next_layer_socket);
    return *this;
  }

  socket_context p_socket_context;
  endpoint_type p_local_endpoint;
  endpoint_type p_remote_endpoint;
  next_layer_socket_type p_next_layer_socket;
};

template <class Protocol>
using basic_socket_impl = basic_socket_impl_ex<
    typename Protocol::socket_context, typename Protocol::endpoint,
    typename Protocol::next_layer_protocol::socket>;

template <class Context, class Endpoint, class NextAcceptor>
struct basic_acceptor_impl_ex {
 private:
  typedef std::shared_ptr<Context> acceptor_context;
  typedef std::shared_ptr<Endpoint> endpoint_type;
  typedef std::shared_ptr<NextAcceptor> next_layer_acceptor_type;

 public:
  basic_acceptor_impl_ex()
      : p_acceptor_context(nullptr),
        p_local_endpoint(nullptr),
        p_remote_endpoint(nullptr),
        p_next_layer_acceptor(nullptr) {}

  basic_acceptor_impl_ex(basic_acceptor_impl_ex&& other)
      : p_acceptor_context(std::move(other.p_acceptor_context)),
        p_local_endpoint(std::move(other.p_local_endpoint)),
        p_remote_endpoint(std::move(other.p_remote_endpoint)),
        p_next_layer_acceptor(std::move(other.p_next_layer_acceptor)) {}

  basic_acceptor_impl_ex& operator=(basic_acceptor_impl_ex&& other) {
    p_acceptor_context = std::move(other.p_acceptor_context);
    p_local_endpoint = std::move(other.p_local_endpoint);
    p_remote_endpoint = std::move(other.p_remote_endpoint);
    p_next_layer_acceptor = std::move(other.p_next_layer_acceptor);
    return *this;
  }

  acceptor_context p_acceptor_context;
  endpoint_type p_local_endpoint;
  endpoint_type p_remote_endpoint;
  next_layer_acceptor_type p_next_layer_acceptor;
};

template <class Protocol>
using basic_acceptor_impl = basic_acceptor_impl_ex<
    typename Protocol::acceptor_context, typename Protocol::endpoint,
    typename Protocol::next_layer_protocol::acceptor>;

}  // layer
}  // ssf

#endif  // SSF_LAYER_BASIC_IMPL_H_
