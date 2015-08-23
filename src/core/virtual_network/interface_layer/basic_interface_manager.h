#ifndef SSF_CORE_VIRTUAL_NETWORK_INTERFACE_LAYER_BASIC_INTERFACE_MANAGER_H_
#define SSF_CORE_VIRTUAL_NETWORK_INTERFACE_LAYER_BASIC_INTERFACE_MANAGER_H_

#include <map>
#include <memory>

#include <boost/thread/recursive_mutex.hpp>

namespace virtual_network {
namespace interface_layer {

template <class Protocol>
struct basic_InterfaceManager {
  typedef std::shared_ptr<typename Protocol::socket> SocketPtr;
  typedef typename Protocol::endpoint_context_type EndpointContext;

  basic_InterfaceManager() : mutex(), available_sockets() {}

  boost::recursive_mutex mutex;
  std::map<EndpointContext, SocketPtr> available_sockets;
};

}  // interface_layer
}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_INTERFACE_LAYER_BASIC_INTERFACE_MANAGER_H_