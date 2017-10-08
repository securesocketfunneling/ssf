#ifndef SSF_LAYER_INTERFACE_LAYER_BASIC_INTERFACE_MANAGER_H_
#define SSF_LAYER_INTERFACE_LAYER_BASIC_INTERFACE_MANAGER_H_

#include <map>
#include <memory>
#include <mutex>

#include <boost/optional.hpp>

namespace ssf {
namespace layer {
namespace interface_layer {

template <class Protocol>
class basic_InterfaceManager {
 public:
  typedef std::shared_ptr<typename Protocol::socket> SocketPtr;
  typedef typename Protocol::endpoint_context_type EndpointContext;

 public:
  basic_InterfaceManager() : mutex_(), available_sockets_() {}

  std::size_t Count(const EndpointContext& endpoint) {
    return available_sockets_.count(endpoint);
  }

  boost::optional<SocketPtr> Find(const EndpointContext& endpoint) {
    std::unique_lock<std::mutex> lock_sockets(mutex_);
    auto p_socket_it = available_sockets_.find(endpoint);
    if (p_socket_it != available_sockets_.end()) {
      return boost::optional<SocketPtr>(p_socket_it->second);
    }
    return boost::optional<SocketPtr>();
  }

  bool Emplace(const EndpointContext& endpoint, SocketPtr p_socket) {
    std::unique_lock<std::mutex> lock_sockets(mutex_);
    auto inserted = available_sockets_.emplace(endpoint, p_socket);

    return inserted.second;
  }

  bool Erase(const EndpointContext& endpoint) {
    std::unique_lock<std::mutex> lock_sockets(mutex_);
    return 1 == available_sockets_.erase(endpoint);
  }

 private:
  std::mutex mutex_;
  std::map<EndpointContext, SocketPtr> available_sockets_;
};

}  // interface_layer
}  // layer
}  // ssf

#endif  // SSF_LAYER_INTERFACE_LAYER_BASIC_INTERFACE_MANAGER_H_