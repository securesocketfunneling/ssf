#ifndef SSF_LAYER_DATA_LINK_BASIC_CIRCUIT_ACCEPTOR_SERVICE_H_
#define SSF_LAYER_DATA_LINK_BASIC_CIRCUIT_ACCEPTOR_SERVICE_H_

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <thread>
#include <type_traits>
#include <utility>

#include <boost/asio/detail/op_queue.hpp>
#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

#include "ssf/error/error.h"

#include "ssf/io/accept_op.h"
#include "ssf/io/handler_helpers.h"

#include "ssf/layer/basic_impl.h"
#include "ssf/layer/data_link/circuit_op.h"
#include "ssf/layer/data_link/helpers.h"
#include "ssf/layer/parameters.h"

#include "ssf/log/log.h"

#include "ssf/network/base_session.h"
#include "ssf/network/manager.h"
#include "ssf/network/session_forwarder.h"

namespace ssf {
namespace layer {
namespace data_link {

#include <boost/asio/detail/push_options.hpp>

template <class Prococol>
class basic_CircuitAcceptor_service
    : public boost::asio::detail::service_base<
          basic_CircuitAcceptor_service<Prococol>> {
 public:
  typedef Prococol protocol_type;

  typedef typename protocol_type::endpoint endpoint_type;
  typedef typename protocol_type::resolver resolver_type;

  typedef std::shared_ptr<endpoint_type> p_endpoint_type;

  typedef basic_acceptor_impl<protocol_type> implementation_type;
  typedef implementation_type& native_handle_type;
  typedef native_handle_type native_type;

 private:
  typedef typename protocol_type::circuit_policy circuit_policy;
  typedef ssf::ItemManager<ssf::BaseSessionPtr> Manager;
  typedef typename protocol_type::socket socket_type;
  typedef
      typename protocol_type::next_layer_protocol::acceptor next_acceptor_type;
  typedef typename protocol_type::next_layer_protocol::socket next_socket_type;
  typedef
      typename protocol_type::next_layer_protocol::endpoint next_endpoint_type;
  typedef std::shared_ptr<socket_type> p_socket_type;
  typedef std::shared_ptr<next_acceptor_type> p_next_acceptor_type;
  typedef std::shared_ptr<next_socket_type> p_next_socket_type;
  typedef std::shared_ptr<next_endpoint_type> p_next_endpoint_type;

  typedef std::pair<p_next_socket_type, p_endpoint_type> pending_connection;
  typedef std::queue<pending_connection> connection_queue;
  typedef boost::asio::detail::op_queue<
      io::basic_pending_accept_operation<protocol_type>>
      op_queue;

 public:
  explicit basic_CircuitAcceptor_service(boost::asio::io_service& io_service)
      : boost::asio::detail::service_base<basic_CircuitAcceptor_service>(
            io_service),
        next_acceptors_(),
        next_local_endpoints_() {}

  virtual ~basic_CircuitAcceptor_service() {}

  void construct(implementation_type& impl) {
    impl.p_next_layer_acceptor =
        std::make_shared<next_acceptor_type>(this->get_io_service());
  }

  void destroy(implementation_type& impl) {
    impl.p_local_endpoint.reset();
    impl.p_remote_endpoint.reset();
    impl.p_next_layer_acceptor.reset();
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
    return impl.p_next_layer_acceptor->open(
        typename protocol_type::next_layer_protocol(), ec);
  }

  boost::system::error_code assign(implementation_type& impl,
                                   const protocol_type& protocol,
                                   native_handle_type& native_socket,
                                   boost::system::error_code& ec) {
    impl = native_socket;
    return ec;
  }

  bool is_open(const implementation_type& impl) const {
    return impl.p_next_layer_acceptor->is_open();
  }

  endpoint_type remote_endpoint(const implementation_type& impl,
                                boost::system::error_code& ec) const {
    if (impl.p_remote_endpoint) {
      return *impl.p_remote_endpoint;
    } else {
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
    if (impl.p_next_layer_acceptor) {
      impl.p_next_layer_acceptor->close(ec);
    }

    {
      std::unique_lock<std::recursive_mutex> lock(bind_mutex_);

      endpoint_type input_endpoint;
      for (const auto& pair : input_bindings_) {
        if (pair.second == &impl) {
          input_endpoint = pair.first;

          {
            std::unique_lock<std::recursive_mutex> lock1(accept_mutex_);
            pending_connections_.erase(
                impl.p_local_endpoint->next_layer_endpoint());
          }

          break;
        }
      }
      input_bindings_.erase(input_endpoint);

      endpoint_type forward_endpoint;
      for (const auto& pair : forward_bindings_) {
        if (pair.second == &impl) {
          forward_endpoint = pair.first;
          break;
        }
      }
      forward_bindings_.erase(forward_endpoint);

      p_next_acceptor_type p_input_next_acceptor;
      auto input_next_acceptor_it =
          next_acceptors_.find(input_endpoint.next_layer_endpoint());
      if (input_next_acceptor_it != std::end(next_acceptors_)) {
        p_input_next_acceptor = input_next_acceptor_it->second;
        next_acceptors_.erase(input_endpoint.next_layer_endpoint());
      }

      p_next_acceptor_type p_forward_next_acceptor;
      auto forward_next_acceptor_it =
          next_acceptors_.find(forward_endpoint.next_layer_endpoint());
      if (forward_next_acceptor_it != std::end(next_acceptors_)) {
        p_forward_next_acceptor = forward_next_acceptor_it->second;
        next_acceptors_.erase(forward_endpoint.next_layer_endpoint());
      }

      next_local_endpoints_.erase(p_input_next_acceptor);
      next_local_endpoints_.erase(p_forward_next_acceptor);

      listening_.erase(p_input_next_acceptor);
      listening_.erase(p_forward_next_acceptor);

      if (p_input_next_acceptor) {
        clean_pending_accepts(
            input_endpoint.next_layer_endpoint(),
            boost::system::error_code(ssf::error::interrupted,
                                      ssf::error::get_ssf_category()));
        p_input_next_acceptor->close(ec);
      }

      if (p_forward_next_acceptor) {
        p_forward_next_acceptor->close(ec);
      }
    }

    // close forwarding sessions
    manager_.stop_all();

    return ec;
  }

  native_type native(implementation_type& impl) { return impl; }

  native_handle_type native_handle(implementation_type& impl) { return impl; }

  // Set a socket option.
  template <typename SettableSocketOption>
  boost::system::error_code set_option(implementation_type& impl,
                                       const SettableSocketOption& option,
                                       boost::system::error_code& ec) {
    if (impl.p_next_layer_acceptor) {
      return impl.p_next_layer_acceptor->set_option(option, ec);
    }
    return ec;
  }

  // Register given endpoint if not already provided
  //   Create connection queue if not a forward endpoint
  //   Bind the acceptor of the next layer to the next layer endpoint
  //   Link the acceptor of the next layer to the current impl
  boost::system::error_code bind(implementation_type& impl,
                                 const endpoint_type& endpoint,
                                 boost::system::error_code& ec) {
    std::unique_lock<std::recursive_mutex> lock(bind_mutex_);

    auto is_forward = detail::is_endpoint_forwarding(endpoint);

    default_parameters_ = unserialize_parameter_stack(
        endpoint.endpoint_context().default_parameters);
    bool binding_insertion = false;

    if (is_forward) {
      // If the endpoint only forwards, the acceptor will not be able
      //   to accept connections (pending_connections_ not filled)
      auto forward_bind =
          forward_bindings_.emplace(std::make_pair(endpoint, &impl));
      binding_insertion = forward_bind.second;
    }

    if (!is_forward || binding_insertion) {
      auto input_bind =
          input_bindings_.emplace(std::make_pair(endpoint, &impl));
      binding_insertion = input_bind.second;

      std::unique_lock<std::recursive_mutex> lock1(accept_mutex_);
      pending_connections_.emplace(
          std::make_pair(endpoint.next_layer_endpoint(), connection_queue()));
    }

    if (!binding_insertion) {
      if (is_forward &&
          forward_bindings_.find(endpoint) != forward_bindings_.end()) {
        forward_bindings_.erase(endpoint);
      }
      ec.assign(ssf::error::address_in_use, ssf::error::get_ssf_category());
      return ec;
    }

    impl.p_local_endpoint = std::make_shared<endpoint_type>(endpoint);

    auto inserted = next_acceptors_.emplace(
        std::make_pair(impl.p_local_endpoint->next_layer_endpoint(),
                       impl.p_next_layer_acceptor));

    if (!inserted.second) {
      impl.p_next_layer_acceptor = inserted.first->second;
      return boost::system::error_code();
    }

    next_local_endpoints_.emplace(
        std::make_pair(impl.p_next_layer_acceptor,
                       impl.p_local_endpoint->next_layer_endpoint()));

    return impl.p_next_layer_acceptor->bind(
        impl.p_local_endpoint->next_layer_endpoint(), ec);
  }

  // Wait for new connections from next layer if not already listening
  boost::system::error_code listen(implementation_type& impl, int backlog,
                                   boost::system::error_code& ec) {
    std::unique_lock<std::recursive_mutex> lock(bind_mutex_);
    if (listening_.count(impl.p_next_layer_acceptor)) {
      return ec;
    }

    listening_.insert(impl.p_next_layer_acceptor);
    auto listening_ec = impl.p_next_layer_acceptor->listen(backlog, ec);

    if (!ec) {
      start_accepting(impl.p_next_layer_acceptor);
    }

    return listening_ec;
  }

  template <typename Protocol1, typename SocketService>
  boost::system::error_code accept(
      implementation_type& impl,
      boost::asio::basic_socket<Protocol1, SocketService>& peer,
      endpoint_type* p_peer_endpoint, boost::system::error_code& ec,
      typename std::enable_if<
          std::is_convertible<protocol_type, Protocol1>::value>::type* = 0) {
    auto& next_layer_local_endpoint =
        impl.p_local_endpoint->next_layer_endpoint();
    connection_queue* p_queue = nullptr;

    {
      std::unique_lock<std::recursive_mutex> lock(accept_mutex_);
      auto queue_it = pending_connections_.find(next_layer_local_endpoint);

      if (queue_it == std::end(pending_connections_)) {
        ec.assign(ssf::error::bad_address, ssf::error::get_ssf_category());
        return ec;
      }

      p_queue = &queue_it->second;
    }

    // Waiting for new connections from next layer (p_queue is populated
    // asynchronously)
    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      std::unique_lock<std::recursive_mutex> lock(accept_mutex_);
      if (p_queue->empty()) {
        continue;
      }

      auto& peer_impl = peer.native_handle();
      auto connection = std::move(p_queue->front());
      p_queue->pop();
      peer_impl.p_next_layer_socket = connection.first;
      peer_impl.p_remote_endpoint = std::move(connection.second);
      break;
    }

    return ec;
  }

  template <typename Protocol1, typename SocketService, typename AcceptHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(AcceptHandler, void(boost::system::error_code))
  async_accept(
      implementation_type& impl,
      boost::asio::basic_socket<Protocol1, SocketService>& peer,
      endpoint_type* p_peer_endpoint, AcceptHandler&& handler,
      typename std::enable_if<
          std::is_convertible<protocol_type, Protocol1>::value>::type* = 0) {
    boost::asio::detail::async_result_init<AcceptHandler,
                                           void(boost::system::error_code)>
        init(std::forward<AcceptHandler>(handler));

    if (!impl.p_local_endpoint) {
      io::PostHandler(
          this->get_io_service(), init.handler,
          boost::system::error_code(ssf::error::identifier_removed,
                                    ssf::error::get_ssf_category()));
      return init.result.get();
    }

    auto& peer_impl = peer.native_handle();
    peer_impl.p_local_endpoint = impl.p_local_endpoint;
    peer_impl.p_remote_endpoint =
        std::make_shared<typename Protocol1::endpoint>();
    peer_impl.p_remote_endpoint->set();

    typedef detail::CircuitAcceptOp<
        typename boost::asio::basic_socket<Protocol1,
                                           SocketService>::native_handle_type,
        endpoint_type,
        typename boost::asio::handler_type<
            AcceptHandler, void(boost::system::error_code)>::type>
        CircuitAcceptOp;
    CircuitAcceptOp op_handler(peer_impl, p_peer_endpoint, init.handler);

    {
      std::unique_lock<std::recursive_mutex> lock(accept_mutex_);
      // Create and save a new op_queue at
      // [impl.p_local_endpoint->next_layer_endpoint()] index
      // in pending_accepts
      auto& op_queue =
          pending_accepts_[impl.p_local_endpoint->next_layer_endpoint()];

      typedef io::pending_accept_operation<CircuitAcceptOp, protocol_type> op;
      typename op::ptr p = {
          boost::asio::detail::addressof(op_handler),
          boost_asio_handler_alloc_helpers::allocate(sizeof(op), op_handler),
          0};

      p.p = new (p.v)
          op(peer, peer_impl.p_remote_endpoint.get(), std::move(op_handler));
      op_queue.push(p.p);
      p.v = p.p = 0;
    }
    connection_queue_handler();

    return init.result.get();
  }

 private:
  // Start async accepting new connection on the next layer
  void start_accepting(p_next_acceptor_type p_next_layer_acceptor,
                       p_next_socket_type p_next_layer_socket = nullptr,
                       p_next_endpoint_type p_next_layer_endpoint = nullptr) {
    if (p_next_layer_acceptor->is_open()) {
      if (!p_next_layer_socket) {
        p_next_layer_socket = std::make_shared<
            typename protocol_type::next_layer_protocol::socket>(
            this->get_io_service());
      }

      if (!p_next_layer_endpoint) {
        p_next_layer_endpoint = std::make_shared<
            typename protocol_type::next_layer_protocol::endpoint>();
      }

      auto on_accept =
          [this, p_next_layer_acceptor, p_next_layer_socket,
           p_next_layer_endpoint](const boost::system::error_code& ec) {
            accepted(p_next_layer_acceptor, p_next_layer_socket,
                     p_next_layer_endpoint, ec);
          };
      p_next_layer_acceptor->async_accept(*p_next_layer_socket,
                                          *p_next_layer_endpoint, on_accept);
    }
  }

  /// New connection was accepted by the next layer
  void accepted(p_next_acceptor_type p_next_layer_acceptor,
                p_next_socket_type p_next_layer_socket,
                p_next_endpoint_type p_next_layer_endpoint,
                const boost::system::error_code& ec) {
    // Do not break accept loop
    start_accepting(p_next_layer_acceptor);

    if (ec) {
      SSF_LOG("network_link", debug, "could not accept connection ({})",
              ec.message());
      return;
    }

    {
      std::unique_lock<std::recursive_mutex> lock2(bind_mutex_);
      std::unique_lock<std::recursive_mutex> lock1(accept_mutex_);

      auto next_local_endpoint_it =
          next_local_endpoints_.find(p_next_layer_acceptor);

      if (next_local_endpoint_it == std::end(next_local_endpoints_)) {
        return;
      }

      auto& next_local_endpoint = next_local_endpoint_it->second;

      // Create empty endpoint to store the future remote endpoint received by
      // the circuit protocol
      auto p_remote_endpoint =
          std::make_shared<endpoint_type>(*p_next_layer_endpoint);

      auto p_received_endpoint = std::make_shared<endpoint_type>();

      p_received_endpoint->endpoint_context().default_parameters =
          serialize_parameter_stack(default_parameters_);

      auto on_init_connection =
          [this, p_next_layer_socket, p_remote_endpoint, p_received_endpoint,
           next_local_endpoint](const boost::system::error_code& ec) {
            connection_initiated_handler(p_next_layer_socket, p_remote_endpoint,
                                         p_received_endpoint,
                                         next_local_endpoint, ec);
          };
      circuit_policy::AsyncInitConnection(
          *p_next_layer_socket, p_received_endpoint.get(),
          circuit_policy::server, on_init_connection);
    }
  }

  void connection_initiated_handler(p_next_socket_type p_next_layer_socket,
                                    p_endpoint_type p_remote_endpoint,
                                    p_endpoint_type p_received_endpoint,
                                    next_endpoint_type next_local_endpoint,
                                    const boost::system::error_code& ec) {
    if (ec) {
      SSF_LOG("network_link", debug, "connection not initialized ({})",
              ec.message());
      boost::system::error_code close_ec;
      p_next_layer_socket->shutdown(boost::asio::socket_base::shutdown_both,
                                    close_ec);
      p_next_layer_socket->close(close_ec);
      return;
    }

    if (detail::is_endpoint_forwarding(*p_received_endpoint)) {
      // Received endpoint is not the final destination :
      //   create and connect a new socket to the next endpoint
      //   and forward its data
      do_connection_forward(std::move(p_next_layer_socket),
                            std::move(p_received_endpoint));
      return;
    }

    p_remote_endpoint->endpoint_context() =
        p_received_endpoint->endpoint_context();

    p_remote_endpoint->endpoint_context().id =
        p_remote_endpoint->endpoint_context().details;

    std::unique_lock<std::recursive_mutex> lock1(accept_mutex_);

    // Connection is only accepted if there is a binding
    // of a non forward endpoint link to this next local endpoint
    if (pending_connections_.count(next_local_endpoint)) {
      auto on_connection_validated = [this, next_local_endpoint,
                                      p_next_layer_socket, p_remote_endpoint](
          const boost::system::error_code& ec) {
        connection_validated_handler(next_local_endpoint, p_next_layer_socket,
                                     p_remote_endpoint, ec);
      };
      circuit_policy::AsyncValidateConnection(
          *p_next_layer_socket, p_remote_endpoint.get(), ec.value(),
          on_connection_validated);
    } else {
      boost::system::error_code close_ec;
      p_next_layer_socket->shutdown(boost::asio::socket_base::shutdown_both,
                                    close_ec);
      p_next_layer_socket->close(close_ec);
    }
  }

  void connection_validated_handler(next_endpoint_type next_local_endpoint,
                                    p_next_socket_type p_next_layer_socket,
                                    p_endpoint_type p_remote_endpoint,
                                    const boost::system::error_code& ec) {
    if (ec) {
      SSF_LOG("network_link", debug, "connection not valid ({})", ec.message());
      boost::system::error_code close_ec;
      p_next_layer_socket->shutdown(boost::asio::socket_base::shutdown_both,
                                    close_ec);
      p_next_layer_socket->close(close_ec);
      return;
    }

    this->do_connection_accept(next_local_endpoint,
                               std::make_pair(std::move(p_next_layer_socket),
                                              std::move(p_remote_endpoint)));
  }

  /// Create and connect a new socket to the remote endpoint
  ///   and forward its data
  void do_connection_forward(p_next_socket_type p_next_socket,
                             p_endpoint_type p_remote_endpoint) {
    auto p_forward_socket =
        std::make_shared<socket_type>(this->get_io_service());

    auto on_connect = [this, p_remote_endpoint, p_next_socket,
                       p_forward_socket](const boost::system::error_code& ec) {
      connected_handler(p_remote_endpoint, std::move(p_next_socket),
                        p_forward_socket, ec);
    };
    p_forward_socket->async_connect(*p_remote_endpoint, on_connect);
  }

  void connected_handler(p_endpoint_type p_remote_endpoint,
                         p_next_socket_type p_next_socket,
                         p_socket_type p_forward_socket,
                         const boost::system::error_code& ec) {
    if (ec) {
      // TODO : log error
      circuit_policy::AsyncValidateConnection(
          *p_next_socket, p_remote_endpoint.get(), ec.value(),
          [](const boost::system::error_code&) {});

      boost::system::error_code close_ec;
      p_next_socket->shutdown(boost::asio::socket_base::shutdown_both,
                              close_ec);
      p_next_socket->close(close_ec);
      p_forward_socket->shutdown(boost::asio::socket_base::shutdown_both,
                                 close_ec);
      p_forward_socket->close(close_ec);
      return;
    }

    auto on_connection_forwarded = [this, p_next_socket, p_forward_socket](
        const boost::system::error_code& ec) {
      connection_forwarded_handler(p_next_socket, p_forward_socket, ec);
    };
    circuit_policy::AsyncValidateConnection(*p_next_socket,
                                            p_remote_endpoint.get(), ec.value(),
                                            on_connection_forwarded);
  }

  void connection_forwarded_handler(p_next_socket_type p_next_socket,
                                    p_socket_type p_forward_socket,
                                    const boost::system::error_code& ec) {
    if (ec) {
      // TODO : log error
      boost::system::error_code close_ec;
      p_next_socket->shutdown(boost::asio::socket_base::shutdown_both,
                              close_ec);
      p_next_socket->close(close_ec);
      p_forward_socket->shutdown(boost::asio::socket_base::shutdown_both,
                                 close_ec);
      p_forward_socket->close(close_ec);
      return;
    }

    // pipe data between p_next_socket and p_forward_socket
    auto p_session =
        ssf::SessionForwarder<next_socket_type, next_socket_type>::create(
            &this->manager_,
            std::move(*p_forward_socket->native_handle().p_next_layer_socket),
            std::move(*p_next_socket));

    boost::system::error_code start_ec;
    this->manager_.start(p_session, start_ec);
  }

  /// Unqueue accept operation after accepting connection
  void do_connection_accept(const next_endpoint_type& next_local_endpoint,
                            pending_connection connection) {
    std::unique_lock<std::recursive_mutex> lock(accept_mutex_);

    auto connection_queue_it = pending_connections_.find(next_local_endpoint);

    // Connection is only accepted if there is a binding
    // of a non forward endpoint link to this next local endpoint
    if (connection_queue_it == std::end(pending_connections_)) {
      boost::system::error_code ec;
      connection.first->shutdown(boost::asio::socket_base::shutdown_both, ec);
      connection.first->close(ec);
      return;
    }

    auto& connection_queue = connection_queue_it->second;
    connection_queue.emplace(std::move(connection));

    connection_queue_handler();
  }

  void close_next_layer_acceptor(p_next_acceptor_type p_acceptor) {
    auto endpoint_it = next_local_endpoints_.find(p_acceptor);

    if (endpoint_it == std::end(next_local_endpoints_)) {
      return;
    }

    auto acceptor_it = next_acceptors_.find(endpoint_it->second);

    if (acceptor_it != std::end(next_acceptors_)) {
      boost::system::error_code ec;
      acceptor_it->second->close(ec);
    }

    next_local_endpoints_.erase(endpoint_it);
    next_acceptors_.erase(acceptor_it);

    return;
  }

  /// Execute accept handler after peer connection
  void connection_queue_handler(
      const boost::system::error_code& ec = boost::system::error_code()) {
    std::unique_lock<std::recursive_mutex> lock(accept_mutex_);

    for (auto& connection_pair : pending_connections_) {
      if (!connection_pair.second.size()) {
        continue;
      }

      auto accept_queue_it = pending_accepts_.find(connection_pair.first);

      if (accept_queue_it == std::end(pending_accepts_)) {
        continue;
      }

      auto& connection_queue = connection_pair.second;
      auto& accept_queue = accept_queue_it->second;
      // auto next_local_endpoint = connection_pair.first;

      if (!connection_queue.empty() && !accept_queue.empty()) {
        auto connection = std::move(connection_queue.front());
        connection_queue.pop();
        auto* p_accept_op = accept_queue.front();
        accept_queue.pop();

        if (!ec) {
          p_accept_op->set_p_endpoint(*connection.second);
          auto& peer = p_accept_op->peer();
          auto& native_handle = peer.native_handle();
          native_handle.p_next_layer_socket = std::move(connection.first);
          native_handle.p_remote_endpoint = connection.second;
        }

        auto do_complete = [p_accept_op, ec]() { p_accept_op->complete(ec); };
        this->get_io_service().post(do_complete);

        auto on_connection_queue = [this, ec]() {
          connection_queue_handler(ec);
        };
        this->get_io_service().post(on_connection_queue);
      }
    }
  }

  void clean_pending_accepts(const next_endpoint_type& next_layer_endpoint,
                             const boost::system::error_code& ec) {
    std::unique_lock<std::recursive_mutex> lock(accept_mutex_);
    auto accept_queue_it = pending_accepts_.find(next_layer_endpoint);

    if (accept_queue_it == pending_accepts_.end()) {
      return;
    }

    auto& accept_queue = accept_queue_it->second;
    while (!accept_queue.empty()) {
      auto* p_accept_op = accept_queue.front();
      accept_queue.pop();
      p_accept_op->complete(ec);
    }

    pending_accepts_.erase(next_layer_endpoint);
  }

  void shutdown_service() { manager_.stop_all(); }

 private:
  std::recursive_mutex bind_mutex_;
  ParameterStack default_parameters_;
  std::map<next_endpoint_type, p_next_acceptor_type> next_acceptors_;
  std::map<p_next_acceptor_type, next_endpoint_type> next_local_endpoints_;
  std::map<endpoint_type, implementation_type*> input_bindings_;
  std::map<endpoint_type, implementation_type*> forward_bindings_;
  std::set<p_next_acceptor_type> listening_;

  std::recursive_mutex accept_mutex_;
  std::map<next_endpoint_type, op_queue> pending_accepts_;
  std::map<next_endpoint_type, connection_queue> pending_connections_;

  Manager manager_;
};

#include <boost/asio/detail/pop_options.hpp>

}  // data_link
}  // layer
}  // ssf

#endif  // SSF_LAYER_DATA_LINK_BASIC_CIRCUIT_ACCEPTOR_SERVICE_H_
