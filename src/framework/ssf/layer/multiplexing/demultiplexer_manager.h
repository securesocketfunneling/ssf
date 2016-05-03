#ifndef SSF_LAYER_MULTIPLEXING_DEMULTIPLEXER_MANAGER_H_
#define SSF_LAYER_MULTIPLEXING_DEMULTIPLEXER_MANAGER_H_

#include <memory>

#include <boost/thread/recursive_mutex.hpp>

#include "ssf/layer/multiplexing/basic_demultiplexer.h"

namespace ssf {
namespace layer {
namespace multiplexing {

template <class Protocol, class CongestionPolicy>
class DemultiplexerManager {
 private:
  typedef basic_Demultiplexer<Protocol, CongestionPolicy> Demultiplexer;
  typedef basic_DemultiplexerPtr<Protocol, CongestionPolicy> DemultiplexerPtr;
  typedef typename Protocol::next_layer_protocol::socket NextSocket;
  typedef std::shared_ptr<NextSocket> NextSocketPtr;
  typedef typename Protocol::socket_context SocketContext;
  typedef std::shared_ptr<SocketContext> SocketContextPtr;

 public:
  bool Start(NextSocketPtr p_socket) {
    boost::recursive_mutex::scoped_lock lock(mutex_);

    auto inserted =
      demultiplexers_.insert(std::make_pair(p_socket, Demultiplexer::Create(p_socket)));
    inserted.first->second->Start();
    return inserted.second;
  }

  void Stop(NextSocketPtr p_socket) {
    boost::recursive_mutex::scoped_lock lock(mutex_);

    auto demultiplexer_it = demultiplexers_.find(p_socket);

    if (demultiplexer_it != std::end(demultiplexers_)) {
      demultiplexer_it->second->Stop();
      demultiplexers_.erase(demultiplexer_it);
    }
  }

  void Stop() {
    boost::recursive_mutex::scoped_lock lock(mutex_);

    for (auto& pair : demultiplexers_) {
      pair.second->Stop();
    }

    demultiplexers_.clear();
  }

  bool Bind(NextSocketPtr p_socket, SocketContextPtr p_socket_context) {
    boost::recursive_mutex::scoped_lock lock(mutex_);
    // Get the demultiplexer linked to the given socket
    auto demultiplexer_it = demultiplexers_.find(p_socket);

    if (demultiplexer_it == std::end(demultiplexers_)) {
      return false;
    }

    return demultiplexer_it->second->Bind(p_socket_context);
  }

  bool Unbind(NextSocketPtr p_socket, SocketContextPtr p_socket_context) {
    boost::recursive_mutex::scoped_lock lock(mutex_);
    // Get the demultiplexer linked to the given socket
    auto demultiplexer_it = demultiplexers_.find(p_socket);

    if (demultiplexer_it == std::end(demultiplexers_)) {
      return false;
    }

    return demultiplexer_it->second->Unbind(p_socket_context);
  }

  bool IsBound(NextSocketPtr p_socket, SocketContextPtr p_socket_context) {
    boost::recursive_mutex::scoped_lock lock(mutex_);
    // Get the demultiplexer linked to the given socket
    auto demultiplexer_it = demultiplexers_.find(p_socket);

    if (demultiplexer_it == std::end(demultiplexers_)) {
      return false;
    }

    return demultiplexer_it->second->IsBound(p_socket_context);
  }

  void Read(NextSocketPtr p_socket, SocketContextPtr p_socket_context) {
    boost::recursive_mutex::scoped_lock lock(mutex_);
    // Get the demultiplexer linked to the given socket
    auto demultiplexer_it = demultiplexers_.find(p_socket);

    if (demultiplexer_it == std::end(demultiplexers_)) {
      return;
    }

    return demultiplexer_it->second->Read(p_socket_context);
  }

 private:
  boost::recursive_mutex mutex_;
  std::map<NextSocketPtr, DemultiplexerPtr> demultiplexers_;
};

}  // multiplexing
}  // layer
}  // ssf

#endif  // SSF_LAYER_MULTIPLEXING_DEMULTIPLEXER_MANAGER_H_
