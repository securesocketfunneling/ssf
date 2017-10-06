#ifndef SSF_SERVICES_DATAGRAM_DATAGRAM_LINK_H_
#define SSF_SERVICES_DATAGRAM_DATAGRAM_LINK_H_

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>

#include <boost/noncopyable.hpp>  // NOLINT

#include <boost/asio/buffer.hpp>  // NOLINT
#include <boost/asio/coroutine.hpp>
#include <boost/system/error_code.hpp>  // NOLINT

#include <boost/asio/steady_timer.hpp>

#include "services/datagram/datagram_link_operator.h"

namespace ssf {
//-----------------------------------------------------------------------------

/// Asynchronous Full Duplex Datagram Forwarder
template <class RemoteEndpointRight, class RightEndSocket,
          class RemoteEndpointLeft, class LeftEndSocket>
struct DatagramLink : public std::enable_shared_from_this<
                          DatagramLink<RemoteEndpointRight, RightEndSocket,
                                       RemoteEndpointLeft, LeftEndSocket>> {
 private:
  template <class T>
  struct make_queue {
    typedef std::queue<T> type;
    typedef T value_type;
  };

  typedef std::shared_ptr<DatagramLink> DatagramLinkPtr;

  /// Types for the DatagramLink manager
  typedef DatagramLinkOperator<RemoteEndpointRight, RightEndSocket,
                               RemoteEndpointLeft, LeftEndSocket>
      DatagramLinkOperatorSpec;
  typedef std::shared_ptr<DatagramLinkOperatorSpec> DatagramLinkOperatorSpecPtr;

  typedef std::vector<uint8_t> Datagram;
  typedef typename make_queue<Datagram>::type DatagramQueue;

 public:
  static DatagramLinkPtr Create(RightEndSocket& right, LeftEndSocket left,
                                RemoteEndpointRight remote_endpoint_right,
                                RemoteEndpointLeft remote_endpoint_left,
                                boost::asio::io_service& io_service,
                                DatagramLinkOperatorSpecPtr oper) {
    return DatagramLinkPtr(
        new DatagramLink(right, std::move(left), remote_endpoint_right,
                         remote_endpoint_left, io_service, oper));
  }

  /// Feed data to be sent to the "left endpoint"
  void Feed(boost::asio::const_buffer buffer, size_t length) {
    std::unique_lock<std::recursive_mutex> lock(datagram_queue_mutex_);

    // Copy the data into the Datagram queue
    Datagram dgr(length);
    boost::asio::buffer_copy(boost::asio::buffer(dgr), buffer);
    dgr_queue_.push(dgr);

    // Notify that new data is available
    timer_.cancel();
  }

#include <boost/asio/yield.hpp>  // NOLINT
  /// Loop that sends data to the "left endpoint"
  void ExternalFeed(
      const boost::system::error_code& ec = boost::system::error_code(),
      std::size_t n = 0) {
    std::unique_lock<std::recursive_mutex> lock(datagram_queue_mutex_);

    // Stop the loop if stopping the DatagramLink
    if (stopping_) {
      return;
    }

    reenter(coro_external_) {
      for (;;) {
        // If no error occured but no data is available, we wait
        if (!ec && dgr_queue_.empty()) {
          timer_.expires_from_now(std::chrono::seconds(120));
          yield timer_.async_wait(std::bind(&DatagramLink::ExternalFeed,
                                            this->shared_from_this(),
                                            std::placeholders::_1, 0));
        }

        // if no other error than a canceled timer has occured and that some
        // data is available, we send it, else we stop
        if ((!ec || ec.value() == boost::asio::error::operation_aborted) &&
            !dgr_queue_.empty()) {
          if (l_.is_open()) {
            yield l_.async_send_to(
                boost::asio::buffer(dgr_queue_.front()), remote_endpoint_left_,
                std::bind(&DatagramLink::ExternalFeed, this->shared_from_this(),
                          std::placeholders::_1, std::placeholders::_2));
            dgr_queue_.pop();
            if (!auto_feeding_) {
              auto_feeding_ = true;
              AutoFeed();
            }
          } else {
            Shutdown();
            return;
          }
        } else {
          Shutdown();
          return;
        }
      }
    }
  }
#include <boost/asio/unyield.hpp>  // NOLINT

#include <boost/asio/yield.hpp>  // NOLINT
  /// Loop that automatically forward the data from left to right
  void AutoFeed(
      const boost::system::error_code& ec = boost::system::error_code(),
      std::size_t n = 0) {
    if (stopping_) {
      return;
    }
    if (!ec && r_.is_open() && l_.is_open()) reenter(coro_auto_) {
        for (;;) {
          yield l_.async_receive_from(
              boost::asio::buffer(array_buffer_), remote_endpoint_left_,
              std::bind(&DatagramLink::AutoFeed, this->shared_from_this(),
                        std::placeholders::_1, std::placeholders::_2));
          bytes_to_transfer_ = n;
          transferred_bytes_ = 0;
          while (transferred_bytes_ < bytes_to_transfer_) {
            yield r_.async_send_to(
                boost::asio::buffer(array_buffer_,
                                    bytes_to_transfer_ - transferred_bytes_),
                remote_endpoint_right_,
                std::bind(&DatagramLink::AutoFeed, this->shared_from_this(),
                          std::placeholders::_1, std::placeholders::_2));

            transferred_bytes_ += n;
          }
        }
      }
    else {
      Shutdown();
    }
  }
#include <boost/asio/unyield.hpp>  // NOLINT

  void Stop() {
    boost::system::error_code ec;
    l_.close(ec);
    timer_.cancel(ec);
    stopping_ = true;
  }

 private:
  DatagramLink(RightEndSocket& right, LeftEndSocket left,
               RemoteEndpointRight remote_endpoint_right,
               RemoteEndpointLeft remote_endpoint_left,
               boost::asio::io_service& io_service,
               DatagramLinkOperatorSpecPtr oper)
      : coro_auto_(),
        coro_external_(),
        r_(right),
        l_(std::move(left)),
        bytes_to_transfer_(0),
        transferred_bytes_(0),
        timer_(io_service),
        remote_endpoint_right_(remote_endpoint_right),
        remote_endpoint_left_(remote_endpoint_left),
        stopping_(false),
        p_operator_(oper),
        auto_feeding_(false) {}

  void Shutdown() {
    std::unique_lock<std::recursive_mutex> lock(p_operator_mutex_);
    if (p_operator_ && !stopping_) {
      p_operator_->Stop(this->shared_from_this());
      p_operator_.reset();
    }
  }

 public:
  boost::asio::coroutine coro_auto_;
  boost::asio::coroutine coro_external_;
  RightEndSocket& r_;
  LeftEndSocket l_;
  DatagramQueue dgr_queue_;
  std::array<uint8_t, 50 * 1024> array_buffer_;
  size_t bytes_to_transfer_;
  size_t transferred_bytes_;
  boost::asio::steady_timer timer_;
  RemoteEndpointRight remote_endpoint_right_;
  RemoteEndpointLeft remote_endpoint_left_;
  std::recursive_mutex datagram_queue_mutex_;
  std::recursive_mutex p_operator_mutex_;
  bool stopping_;
  DatagramLinkOperatorSpecPtr p_operator_;
  bool auto_feeding_;
};

}  // ssf

#endif  // SSF_SERVICES_DATAGRAM_DATAGRAM_LINK_H_
