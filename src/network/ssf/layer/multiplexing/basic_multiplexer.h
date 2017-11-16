#ifndef SSF_LAYER_MULTIPLEXING_BASIC_MULTIPLEXER_H_
#define SSF_LAYER_MULTIPLEXING_BASIC_MULTIPLEXER_H_

#include <cstdint>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/asio/write.hpp>

#include "ssf/error/error.h"

#include "ssf/layer/io_handler.h"
#include "ssf/layer/protocol_attributes.h"

namespace ssf {
namespace layer {
namespace multiplexing {

template <class SocketPtr, class Datagram, class Endpoint,
          class CongestionPolicy>
class basic_Multiplexer
    : public std::enable_shared_from_this<
          basic_Multiplexer<SocketPtr, Datagram, Endpoint, CongestionPolicy>> {
 private:
  typedef ssf::layer::BaseIOHandler BaseIOHandler;
  typedef ssf::layer::BaseIOHandlerPtr BaseIOHandlerPtr;
  typedef std::pair<std::pair<Endpoint, Datagram>, BaseIOHandlerPtr>
      QueueElement;
  typedef std::queue<QueueElement> Queue;

 public:
  static std::shared_ptr<basic_Multiplexer> Create(SocketPtr p_socket) {
    return std::shared_ptr<basic_Multiplexer>(
        new basic_Multiplexer(std::move(p_socket)));
  }

  template <class Handler>
  bool Send(Datagram datagram, const Endpoint& destination, Handler handler) {
    if (!ready_) {
      return false;
    }

    std::unique_lock<std::recursive_mutex> lock(mutex_);

    if (!congestion_policy_.IsAddable(pending_datagrams_, datagram)) {
      // drop packet and notify handler
      p_socket_->get_io_service().post([handler]() {
        handler(boost::system::error_code(ssf::error::no_buffer_space,
                                          ssf::error::get_ssf_category()),
                0);
      });
      return false;
    }

    pending_datagrams_.emplace(std::make_pair(
        std::make_pair(destination, std::move(datagram)),
        std::make_shared<IOHandler<Handler>>(std::move(handler))));

    if (!popping_) {
      StartPopping();
    }

    return true;
  }

  void Stop() { ready_ = false; }

 private:
  basic_Multiplexer(SocketPtr p_socket)
      : ready_(true),
        popping_(false),
        p_socket_(std::move(p_socket)),
        mutex_(),
        pending_datagrams_(),
        congestion_policy_() {}

  /// Get the first element of the datagram queue and async send it
  void StartPopping() {
    std::unique_lock<std::recursive_mutex> lock(mutex_);

    if (pending_datagrams_.empty()) {
      popping_ = false;
      return;
    }

    popping_ = true;
    auto& datagram = pending_datagrams_.front();

    AsyncSendDatagram(
        *p_socket_, datagram.first.second, datagram.first.first,
        std::bind(&basic_Multiplexer::DatagramSent, this->shared_from_this(),
                  std::placeholders::_1, std::placeholders::_2));
  }

  /// Pop an element from the datagram queue
  void DatagramSent(const boost::system::error_code& ec, std::size_t length) {
    {
      std::unique_lock<std::recursive_mutex> lock(mutex_);
      auto datagram = std::move(pending_datagrams_.front());
      pending_datagrams_.pop();
      auto p_handler = std::move(datagram.second);
      p_socket_->get_io_service().post(
          [p_handler, ec, length]() { (*p_handler)(ec, length); });
    }

    if (!ec) {
      StartPopping();
    } else {
      ready_ = false;
      popping_ = false;
    }
  }

 private:
  std::atomic<bool> ready_;
  std::atomic<bool> popping_;
  SocketPtr p_socket_;
  std::recursive_mutex mutex_;
  Queue pending_datagrams_;
  CongestionPolicy congestion_policy_;
};

template <class SocketPtr, class Datagram, class Endpoint,
          class CongestionPolicy>
using basic_MultiplexerPtr = std::shared_ptr<
    basic_Multiplexer<SocketPtr, Datagram, Endpoint, CongestionPolicy>>;

}  // multiplexing
}  // layer
}  // ssf

#endif  // SSF_LAYER_MULTIPLEXING_BASIC_MULTIPLEXER_H_
