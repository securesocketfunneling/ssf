#ifndef SSF_LAYER_MULTIPLEXING_MULTIPLEXER_MANAGER_H_
#define SSF_LAYER_MULTIPLEXING_MULTIPLEXER_MANAGER_H_

#include <map>

#include <boost/thread/recursive_mutex.hpp>

#include "ssf/layer/multiplexing/basic_multiplexer.h"

namespace ssf {
namespace layer {
namespace multiplexing {

template <class SocketPtr, class Datagram, class Endpoint,
          class CongestionPolicy>
class MultiplexerManager {
 private:
  typedef basic_Multiplexer<SocketPtr, Datagram, Endpoint, CongestionPolicy> Multiplexer;
  typedef basic_MultiplexerPtr<SocketPtr, Datagram, Endpoint, CongestionPolicy> MultiplexerPtr;

 public:
  MultiplexerManager() : mutex_(), multiplexers_() {}
  ~MultiplexerManager() {}

  bool Start(SocketPtr p_socket) {
    boost::recursive_mutex::scoped_lock lock(mutex_);

    auto inserted =
      multiplexers_.insert(std::make_pair(p_socket, Multiplexer::Create(p_socket)));

    return inserted.second;
  }

  template <class Handler>
  bool Send(SocketPtr p_socket, Datagram datagram, const Endpoint& destination,
            Handler handler) {
    boost::recursive_mutex::scoped_lock lock(mutex_);
    // Get the multiplexer linked to the given socket and send datagram through it
    auto multiplexer_it = multiplexers_.find(p_socket);

    if (multiplexer_it == std::end(multiplexers_)) {
      return false;
    }

    return multiplexer_it->second->Send(std::move(datagram), destination,
                                      std::move(handler));
  }

  void Stop(SocketPtr p_socket) {
    boost::recursive_mutex::scoped_lock lock(mutex_);

    auto multiplexer_it = multiplexers_.find(p_socket);

    if (multiplexer_it != std::end(multiplexers_)) {
      multiplexer_it->second->Stop();
      multiplexers_.erase(multiplexer_it);
    }
  }

  void Stop() {
    boost::recursive_mutex::scoped_lock lock(mutex_);

    for (auto& pair : multiplexers_) {
      pair.second->Stop();
    }

    multiplexers_.clear();
  }

 private:
  boost::recursive_mutex mutex_;
  std::map<SocketPtr, MultiplexerPtr> multiplexers_;
};

}  // multiplexing
}  // layer
}  // ssf

#endif  // SSF_LAYER_MULTIPLEXING_MULTIPLEXER_MANAGER_H_
