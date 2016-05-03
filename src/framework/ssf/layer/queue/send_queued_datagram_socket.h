#ifndef SSF_LAYER_QUEUE_SEND_QUEUED_DATAGRAM_SOCKET_H_
#define SSF_LAYER_QUEUE_SEND_QUEUED_DATAGRAM_SOCKET_H_

#include <cstdint>

#include <atomic>
#include <utility>

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/op_queue.hpp>
#include <boost/asio/detail/throw_error.hpp>
#include <boost/asio/use_future.hpp>

#include <boost/thread/recursive_mutex.hpp>

#include "ssf/io/write_op.h"

#include <boost/asio/detail/push_options.hpp>

namespace ssf {
namespace layer {
namespace queue {

template <class DatagramSocket, uint32_t max_queue_size = 10>
class basic_send_queued_datagram_socket {
 public:
  typedef typename DatagramSocket::protocol_type protocol_type;

  typedef DatagramSocket type;

  /// The type of the next layer.
  typedef DatagramSocket next_layer_type;

  /// The type of the lowest layer.
  typedef typename DatagramSocket::lowest_layer_type lowest_layer_type;

  enum { kMaxQueueSize = max_queue_size };

 private:
  typedef boost::asio::detail::op_queue<io::basic_pending_write_operation>
      OpQueue;

 public:
  /// Construct, passing the specified argument to initialise the next layer.
  template <class... Args>
  explicit basic_send_queued_datagram_socket(Args&&... args)
      : socket_(std::forward<Args>(args)...),
        op_queue_mutex_(),
        op_queue_(),
        op_queue_size_(0),
        popping_mutex_(),
        popping_(false) {}

  basic_send_queued_datagram_socket(const basic_send_queued_datagram_socket&) =
      delete;

  basic_send_queued_datagram_socket& operator=(
      const basic_send_queued_datagram_socket&) = delete;

  /// Get a reference to the next layer.
  next_layer_type& next_layer() { return socket_; }

  /// Get a reference to the lowest layer.
  lowest_layer_type& lowest_layer() { return socket_.lowest_layer(); }

  /// Get a const reference to the lowest layer.
  const lowest_layer_type& lowest_layer() const {
    return socket_.lowest_layer();
  }

  /// Get the io_service associated with the object.
  boost::asio::io_service& get_io_service() { return socket_.get_io_service(); }

  /// Close the stream.
  void close() { socket_.close(); }

  /// Close the stream.
  boost::system::error_code close(boost::system::error_code& ec) {
    return socket_.close(ec);
  }

  /// Write the given data to the stream. Returns the number of bytes written.
  /// Throws an exception on failure.
  template <typename ConstBufferSequence>
  std::size_t send(const ConstBufferSequence& buffers) {
    boost::system::error_code ec;
    auto length = send(buffers, ec);
    boost::asio::detail::throw_error(ec, "write_some");
    return length;
  }

  /// Write the given data to the stream. Returns the number of bytes written,
  /// or 0 if an error occurred and the error handler did not throw.
  template <typename ConstBufferSequence>
  std::size_t send(const ConstBufferSequence& buffers,
                   boost::system::error_code& ec) {
    try {
      ec.clear();
      auto future_value = async_send(buffers, boost::asio::use_future);
      return future_value.get();
    } catch (const std::system_error& e) {
      ec.assign(e.code().value(), ssf::error::get_ssf_category());
      return 0;
    }
  }

  /// Start an asynchronous write. The data being written must be valid for the
  /// lifetime of the asynchronous operation.
  template <typename ConstBufferSequence, typename WriteHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler,
                                void(boost::system::error_code, std::size_t))
      async_send(const ConstBufferSequence& buffers, WriteHandler&& handler) {
    boost::asio::detail::async_result_init<
        WriteHandler, void(boost::system::error_code, std::size_t)>
        init(std::forward<WriteHandler>(handler));

    if (op_queue_size_.load() > kMaxQueueSize) {
      io::PostHandler(
          this->get_io_service(), init.handler,
          boost::system::error_code(ssf::error::buffer_is_full_error,
                                    ssf::error::get_ssf_category()),
          0);

      return init.result.get();
    }

    typedef io::pending_write_operation<ConstBufferSequence, decltype(init.handler)> op;
    typename op::ptr p = {
        boost::asio::detail::addressof(init.handler),
        boost_asio_handler_alloc_helpers::allocate(sizeof(op), init.handler),
        0};

    p.p = new (p.v) op(buffers, init.handler);

    {
      boost::recursive_mutex::scoped_lock lock(op_queue_mutex_);
      ++op_queue_size_;
      op_queue_.push(p.p);
    }

    p.v = p.p = 0;

    StartPopping();

    return init.result.get();
  }

  /// Read some data from the stream. Returns the number of bytes read. Throws
  /// an exception on failure.
  template <typename MutableBufferSequence>
  std::size_t receive(const MutableBufferSequence& buffers) {
    return socket_.receive(buffers);
  }

  /// Read some data from the stream. Returns the number of bytes read or 0 if
  /// an error occurred.
  template <typename MutableBufferSequence>
  std::size_t receive(const MutableBufferSequence& buffers,
                      boost::system::error_code& ec) {
    return socket_.receive(buffers, ec);
  }

  /// Start an asynchronous read. The buffer into which the data will be read
  /// must be valid for the lifetime of the asynchronous operation.
  template <typename MutableBufferSequence, typename ReadHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
                                void(boost::system::error_code, std::size_t))
      async_receive(const MutableBufferSequence& buffers,
                    ReadHandler&& handler) {
    return socket_.async_receive(buffers, std::forward<ReadHandler>(handler));
  }

  /// Peek at the incoming data on the stream. Returns the number of bytes read.
  /// Throws an exception on failure.
  template <typename MutableBufferSequence>
  std::size_t peek(const MutableBufferSequence& buffers) {
    return socket_.peek(buffers);
  }

  /// Peek at the incoming data on the stream. Returns the number of bytes read,
  /// or 0 if an error occurred.
  template <typename MutableBufferSequence>
  std::size_t peek(const MutableBufferSequence& buffers,
                   boost::system::error_code& ec) {
    return socket_.peek(buffers, ec);
  }

  /// Determine the amount of data that may be read without blocking.
  std::size_t in_avail() { return socket_.in_avail(); }

  /// Determine the amount of data that may be read without blocking.
  std::size_t in_avail(boost::system::error_code& ec) {
    return socket_.in_avail(ec);
  }

 private:
  void StartPopping() {
    {
      boost::recursive_mutex::scoped_lock lock(popping_mutex_);
      if (popping_) {
        return;
      }

      popping_ = true;
    }

    Pop();
  }

  void Pop() {
    boost::recursive_mutex::scoped_lock lock(op_queue_mutex_);

    if (op_queue_.empty()) {
      popping_ = false;
      return;
    }

    auto op = op_queue_.front();
    --op_queue_size_;
    op_queue_.pop();

    // TODO : Change lambda to specific handler
    auto complete_lambda =
        [op, this](const boost::system::error_code& ec, std::size_t length) {
          this->socket_.get_io_service().post([this]() { this->Pop(); });
          this->socket_.get_io_service().post(
              [op, ec, length]() { op->complete(ec, length); });
        };

    socket_.async_send(op->const_buffers(), std::move(complete_lambda));
  }

 private:
  DatagramSocket socket_;
  boost::recursive_mutex op_queue_mutex_;
  OpQueue op_queue_;
  std::atomic<uint32_t> op_queue_size_;
  boost::recursive_mutex popping_mutex_;
  bool popping_;
};

}  // queue
}  // layer
}  // ssf

#include <boost/asio/detail/pop_options.hpp>

#endif  // SSF_LAYER_QUEUE_SEND_QUEUED_DATAGRAM_SOCKET_H_
