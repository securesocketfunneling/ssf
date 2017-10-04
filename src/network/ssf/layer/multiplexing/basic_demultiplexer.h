#ifndef SSF_LAYER_MULTIPLEXING_BASIC_DEMULTIPLEXER_H_
#define SSF_LAYER_MULTIPLEXING_BASIC_DEMULTIPLEXER_H_

#include <atomic>
#include <map>
#include <memory>

#include <boost/thread/recursive_mutex.hpp>

#include "ssf/error/error.h"

namespace ssf {
namespace layer {
namespace multiplexing {

template <class Protocol, class CongestionPolicy>
class basic_Demultiplexer : public std::enable_shared_from_this<
          basic_Demultiplexer<Protocol, CongestionPolicy> >
{
 private:
  typedef typename Protocol::next_layer_protocol::socket NextSocket;
  typedef std::shared_ptr<NextSocket> NextSocketPtr;
  typedef typename Protocol::socket_context SocketContext;
  typedef std::shared_ptr<SocketContext> SocketContextPtr;
  typedef typename Protocol::endpoint_context_type EndpointContext;
  typedef typename Protocol::next_layer_protocol::endpoint NextEndpoint;
  typedef std::shared_ptr<NextEndpoint> NextEndpointPtr;
  typedef typename Protocol::ReceiveDatagram ReceiveDatagram;
  typedef std::shared_ptr<ReceiveDatagram> ReceiveDatagramPtr;
  typedef std::shared_ptr<CongestionPolicy> CongestionPolicyPtr;
  typedef std::pair<SocketContextPtr, CongestionPolicyPtr> ContextPtrCongestionPair;

 public:
  static std::shared_ptr<basic_Demultiplexer> Create(NextSocketPtr p_socket) {
    return std::shared_ptr<basic_Demultiplexer>(
      new basic_Demultiplexer(p_socket));
  }

  void Start() {
    reading_ = true;
    AsyncReadHeader();
  }

  void Stop() { reading_ = false; }

  bool Bind(SocketContextPtr p_socket_context) {
    boost::recursive_mutex::scoped_lock lock(mutex_);
    auto& local_multiplexed_map =
        multiplexed_maps_[p_socket_context->local_id];

    auto socket_context_inserted = local_multiplexed_map.emplace(
        p_socket_context->remote_id,
        ContextPtrCongestionPair(p_socket_context,
                                 std::make_shared<CongestionPolicy>()));

    return socket_context_inserted.second;
  }

  bool Unbind(SocketContextPtr p_socket_context) {
    boost::recursive_mutex::scoped_lock lock(mutex_);
    auto& local_multiplexed_map =
            multiplexed_maps_[p_socket_context->local_id];

    local_multiplexed_map.erase(p_socket_context->remote_id);

    if (local_multiplexed_map.empty()) {
      multiplexed_maps_.erase(p_socket_context->local_id);
    }

    return true;
  }

  bool IsBound(SocketContextPtr p_socket_context) {
    boost::recursive_mutex::scoped_lock lock(mutex_);

    auto local_multiplexed_map_it =
          multiplexed_maps_.find(p_socket_context->local_id);

    if (local_multiplexed_map_it == std::end(multiplexed_maps_)) {
      return false;
    }

    auto& local_multiplexed_map = local_multiplexed_map_it->second;
    auto p_context_it =
        local_multiplexed_map.find(p_socket_context->remote_id);

    return (p_context_it != std::end(local_multiplexed_map)) &&
           (p_socket_context == p_context_it->second.first);
  }

  void Read(SocketContextPtr p_socket_context) {
    HandleQueues(p_socket_context);
  }

 private:
  basic_Demultiplexer(NextSocketPtr p_socket)
      : p_socket_(p_socket),
        multiplexed_maps_(),
        mutex_(),
        reading_(false) {}

  /// Async read receive_datagram_type
  void AsyncReadHeader(NextEndpointPtr p_next_endpoint = nullptr,
                       ReceiveDatagramPtr p_datagram = nullptr) {
    if (!reading_) {
      return;
    }

    if (!p_next_endpoint) {
      p_next_endpoint = std::make_shared<NextEndpoint>();
    }

    if (!p_datagram) {
      p_datagram = std::make_shared<ReceiveDatagram>();
    }

    p_datagram->payload().ResetSize();

    AsyncReceiveDatagram(*p_socket_, p_datagram.get(), *p_next_endpoint,
                         boost::bind(&basic_Demultiplexer::DispatchDatagram,
                                     this->shared_from_this(), p_next_endpoint,
                                     p_datagram, _1, _2));
  }

  /// Dispatch the datagram through its corresponding queue
  void DispatchDatagram(NextEndpointPtr p_next_endpoint,
                        ReceiveDatagramPtr p_datagram,
                        const boost::system::error_code& ec,
                        std::size_t length) {
    if (ec) {
      boost::system::error_code close_ec;
      p_socket_->close(close_ec);
      return;
    }

    {
      boost::recursive_mutex::scoped_lock lock(mutex_);

      auto& header = p_datagram->header();
      // Second half header represents the local id
      auto local_id = header.id().GetSecondHalfId();

      auto local_multiplexed_it = multiplexed_maps_.find(local_id);

      if (local_multiplexed_it == std::end(multiplexed_maps_)) {
        return;
      }

      // The local id is multiplexed
      auto& local_multiplexed_map = local_multiplexed_it->second;

      SocketContextPtr p_context = nullptr;
      CongestionPolicyPtr p_congestion_policy = nullptr;

      // First half represents the remote id
      auto remote_id = header.id().GetFirstHalfId();

      auto p_context_it = local_multiplexed_map.find(remote_id);

      if (p_context_it != std::end(local_multiplexed_map)) {
        // There is a socket which is bound on this particular id
        p_context = p_context_it->second.first;
        p_congestion_policy = p_context_it->second.second;
      } else {
        // There is a socket which is bound on any remote id
        auto p_bound_context_it =
          local_multiplexed_map.find(EndpointContext());

        if (p_bound_context_it != std::end(local_multiplexed_map)) {
          p_context = p_bound_context_it->second.first;
          p_congestion_policy = p_bound_context_it->second.second;
        }
      }

      // Enqueue the datagram in the socket context queue. If none, drop it
      if (p_context) {

        {
          boost::recursive_mutex::scoped_lock lock(p_context->mutex);

          auto& datagram_queue = p_context->datagram_queue;
          auto& payload = p_datagram->payload();

          // Drop packet if not addable
          if (p_congestion_policy->IsAddable(datagram_queue, payload)) {
            datagram_queue.push(std::move(*p_datagram));
            auto& next_endpoint_queue = p_context->next_endpoint_queue;
            next_endpoint_queue.push(std::move(*p_next_endpoint));
          }
        }

        HandleQueues(p_context);
      }
    }

    // Continue reading datagram
    AsyncReadHeader(std::move(p_next_endpoint),
                    std::move(p_datagram));
  }

  void HandleQueues(SocketContextPtr p_context,
      const boost::system::error_code& ec = boost::system::error_code()) {
    boost::recursive_mutex::scoped_lock lock(p_context->mutex);
    auto& read_op_queue = p_context->read_op_queue;
    auto& datagram_queue = p_context->datagram_queue;
    auto& next_endpoint_queue = p_context->next_endpoint_queue;

    if (ec) {
      while (!read_op_queue.empty()) {
        auto read_op = std::move(read_op_queue.front());
        read_op_queue.pop();

        auto do_complete = [read_op, ec]() { read_op->complete(ec, 0); };

        p_socket_->get_io_service().post(std::move(do_complete));
      }
      return;
    }

    if (datagram_queue.empty() || read_op_queue.empty()) {
      return;
    }

    auto read_op = std::move(read_op_queue.front());
    read_op_queue.pop();

    auto copied = read_op->fill_buffer(datagram_queue.front());

    if (copied == std::numeric_limits<size_t>::max()) {
      auto do_complete = [read_op]() {
        read_op->complete(
            boost::system::error_code(ssf::error::message_size,
                                      ssf::error::get_ssf_category()),
            0);
      };

      p_socket_->get_io_service().post(std::move(do_complete));
      return;
    }

    auto datagram = std::move(datagram_queue.front());
    datagram_queue.pop();
    auto& id = datagram.header().id();
    auto half_id = Protocol::id_type::MakeHalfRemoteID(id);

    auto src_next_endpoint = std::move(next_endpoint_queue.front());
    next_endpoint_queue.pop();

    read_op->set_p_endpoint(
        Protocol::endpoint(std::move(half_id), std::move(src_next_endpoint)));

    auto do_complete = [read_op, ec, copied]() {
      read_op->complete(ec, copied);
    };

    p_socket_->get_io_service().post(std::move(do_complete));
  }

 private:
  typedef typename Protocol::endpoint_context_type LocalEndpointContext;
  typedef typename Protocol::endpoint_context_type RemoteEndpointContext;
  typedef std::map<RemoteEndpointContext, ContextPtrCongestionPair>
     LocalMultiplexedMaps;
  typedef std::map<LocalEndpointContext, LocalMultiplexedMaps>
     MultiplexedMaps;

 private:
  NextSocketPtr p_socket_;
  MultiplexedMaps multiplexed_maps_;
  boost::recursive_mutex mutex_;
  std::atomic<bool> reading_;
};

template <class Protocol, class CongestionPolicy>
using basic_DemultiplexerPtr =
    std::shared_ptr<basic_Demultiplexer<Protocol, CongestionPolicy>>;

}  // multiplexing
}  // layer
}  // ssf

#endif  // SSF_LAYER_MULTIPLEXING_BASIC_DEMULTIPLEXER_H_
