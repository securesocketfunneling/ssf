#ifndef SSF_LAYER_INTERFACE_LAYER_SPECIFIC_INTERFACE_SOCKET_H_
#define SSF_LAYER_INTERFACE_LAYER_SPECIFIC_INTERFACE_SOCKET_H_

#include <cstdint>

#include <memory>
#include <vector>

#include <boost/asio/detail/op_queue.hpp>

#include <boost/thread.hpp>

#include "ssf/layer/interface_layer/generic_interface_socket.h"
#include "ssf/layer/interface_layer/interface_buffers.h"

#include "ssf/error/error.h"
#include "ssf/io/composed_op.h"
#include "ssf/io/handler_helpers.h"
#include "ssf/io/read_op.h"
#include "ssf/io/write_op.h"

#include "ssf/layer/protocol_attributes.h"

namespace ssf {
namespace layer {
namespace interface_layer {

template <class Protocol, class NextLayerProtocol>
class specific_interface_socket
    : public generic_interface_socket<Protocol>,
      public std::enable_shared_from_this<
          specific_interface_socket<Protocol, NextLayerProtocol>> {
 private:
  using protocol_type = Protocol;
  using internal_socket_type = typename NextLayerProtocol::socket;
  using p_internal_socket_type = std::shared_ptr<internal_socket_type>;
  using endpoint_type = typename protocol_type::endpoint;

  using receive_datagram_type = typename protocol_type::ReceiveDatagram;
  using receive_op_queue_type = boost::asio::detail::op_queue<
      io::basic_pending_read_operation<protocol_type>>;

  using send_datagram_type = typename protocol_type::SendDatagram;
  using send_buffer_type = std::vector<uint8_t>;
  using send_op_queue_type =
      boost::asio::detail::op_queue<io::basic_pending_write_operation>;

 public:
  static std::shared_ptr<specific_interface_socket> Create(
      p_internal_socket_type p_internal_socket) {
    return std::shared_ptr<specific_interface_socket>(
        new specific_interface_socket(std::move(p_internal_socket)));
  }

  virtual std::size_t available(boost::system::error_code& ec) {
    auto available_size = p_internal_socket_->available(ec);
    if (ec) {
      boost::system::error_code close_ec;
      p_internal_socket_->close(close_ec);
    }

    return available_size;
  }

  virtual boost::system::error_code cancel(boost::system::error_code& ec) {
    {
      boost::recursive_mutex::scoped_lock lock(send_mutex_);
      while (!send_op_queue_.empty()) {
        auto op = std::move(send_op_queue_.front());
        send_op_queue_.pop();

        p_internal_socket_->get_io_service().post([op]() {
          op->complete(boost::asio::error::make_error_code(
                           boost::asio::error::operation_aborted),
                       0);
        });
      }
    }

    {
      boost::recursive_mutex::scoped_lock lock(receive_mutex_);

      while (!receive_op_queue_.empty()) {
        auto op = std::move(receive_op_queue_.front());
        receive_op_queue_.pop();

        p_internal_socket_->get_io_service().post([op]() {
          op->complete(boost::asio::error::make_error_code(
                           boost::asio::error::operation_aborted),
                       0);
        });
      }
    }

    return ec;
  }

  virtual void close(boost::system::error_code& ec) {
    boost::recursive_mutex::scoped_lock lock_closed_(closed_mutex_);
    if (!closed_) {
      closed_ = true;
      p_internal_socket_->close(ec);
    }
  }

  virtual void connect(boost::system::error_code& ec) {
    boost::recursive_mutex::scoped_lock lock_closed_(closed_mutex_);
    if (closed_) {
      closed_ = false;
    }

    p_internal_socket_->get_io_service().dispatch(
        [this]() { this->do_async_receive(); });
    p_internal_socket_->get_io_service().dispatch(
        [this]() { this->do_async_send(); });
  }

  virtual void async_receive(interface_mutable_buffers buffers,
                             ssf::layer::WrappedIOHandler handler) {
    boost::recursive_mutex::scoped_lock lock(receive_mutex_);

    typedef io::pending_read_operation<interface_mutable_buffers,
                                       ssf::layer::WrappedIOHandler,
                                       protocol_type> op;
    typename op::ptr p = {
        boost::asio::detail::addressof(handler),
        boost_asio_handler_alloc_helpers::allocate(sizeof(op), handler), 0};

    p.p = new (p.v) op(buffers, handler, nullptr);
    receive_op_queue_.push(p.p);
    p.v = p.p = 0;
    if (!receive_pending_) {
      do_async_receive();
    }
  }

  virtual void async_send(interface_const_buffers buffers,
                          ssf::layer::WrappedIOHandler handler) {
    boost::recursive_mutex::scoped_lock lock(send_mutex_);

    typedef io::pending_write_operation<interface_const_buffers,
                                        ssf::layer::WrappedIOHandler> op;
    typename op::ptr p = {
        boost::asio::detail::addressof(handler),
        boost_asio_handler_alloc_helpers::allocate(sizeof(op), handler), 0};

    p.p = new (p.v) op(buffers, handler);
    send_op_queue_.push(p.p);
    p.v = p.p = 0;

    if (!send_pending_) {
      do_async_send();
    }
  }

 private:
  specific_interface_socket(p_internal_socket_type p_internal_socket)
      : p_internal_socket_(std::move(p_internal_socket)),
        closed_mutex_(),
        closed_(false),
        receive_mutex_(),
        receive_pending_(false),
        internal_receive_datagram_(),
        receive_op_queue_(),
        send_mutex_(),
        send_pending_(false),
        send_buffer_(protocol_type::mtu),
        internal_send_datagram_(
            protocol_type::make_datagram(boost::asio::buffer(send_buffer_))),
        send_op_queue_() {}

  void handle_received(const boost::system::error_code& ec,
                       std::size_t length) {
    if (ec) {
      //  Close socket if any error happened on reading
      boost::system::error_code close_ec;
      this->close(close_ec);
      return;
    }

    {
      boost::recursive_mutex::scoped_lock lock(receive_mutex_);

      if (!receive_op_queue_.empty()) {
        auto op = std::move(receive_op_queue_.front());
        receive_op_queue_.pop();

        boost::system::error_code fill_ec;
        auto copied = op->fill_buffer(internal_receive_datagram_, fill_ec);

        if (fill_ec) {
          auto do_complete =
              [op, fill_ec, copied]() { op->complete(fill_ec, copied); };
          p_internal_socket_->get_io_service().post(std::move(do_complete));
        } else {
          // check copied == length - receive_datagram_type::size ??
          auto do_complete = [op, ec, copied]() { op->complete(ec, copied); };
          p_internal_socket_->get_io_service().post(std::move(do_complete));
        }
      }
    }

    {
      boost::recursive_mutex::scoped_lock lock_closed(closed_mutex_);
      if (!closed_) {
        p_internal_socket_->get_io_service().dispatch(
            [this]() { this->do_async_receive(); });
      }
    }
  }

  void handle_sent(const boost::system::error_code& ec, std::size_t length) {
    if (ec) {
      //  Close socket if any error happened on sending
      boost::system::error_code close_ec;
      this->close(close_ec);
      return;
    }

    {
      boost::recursive_mutex::scoped_lock lock(send_mutex_);

      if (!send_op_queue_.empty()) {
        auto op = std::move(send_op_queue_.front());
        send_op_queue_.pop();

        length -= send_datagram_type::size;

        auto do_complete = [op, ec, length]() { op->complete(ec, length); };
        p_internal_socket_->get_io_service().post(std::move(do_complete));
      }
    }

    {
      boost::recursive_mutex::scoped_lock lock_closed(closed_mutex_);
      if (!closed_) {
        p_internal_socket_->get_io_service().dispatch(
            [this]() { this->do_async_send(); });
      }
    }
  }

  void do_async_receive() {
    {
      boost::recursive_mutex::scoped_lock lock_closed(closed_mutex_);
      if (closed_) {
        return;
      }
    }

    {
      boost::recursive_mutex::scoped_lock lock(receive_mutex_);

      if (receive_op_queue_.empty()) {
        receive_pending_ = false;
        return;
      }

      receive_pending_ = true;
    }

    auto self = this->shared_from_this();
    AsyncReceiveDatagram(
        *p_internal_socket_, &internal_receive_datagram_,
        [self, this](const boost::system::error_code& ec, std::size_t length) {
          this->handle_received(ec, length);
        });
  }

  void do_async_send() {
    {
      boost::recursive_mutex::scoped_lock lock_closed(closed_mutex_);
      if (closed_) {
        return;
      }
    }

    {
      boost::recursive_mutex::scoped_lock lock(send_mutex_);

      if (send_op_queue_.empty()) {
        send_pending_ = false;
        return;
      }

      send_pending_ = true;

      auto op = send_op_queue_.front();
      auto buffers = op->const_buffers();

      if (boost::asio::buffer_size(buffers) > protocol_type::mtu) {
        send_op_queue_.pop();
        auto do_complete = [op]() {
          op->complete(
              boost::system::error_code(ssf::error::message_size,
                                        ssf::error::get_ssf_category()),
              0);
        };
        p_internal_socket_->get_io_service().post(std::move(do_complete));

        p_internal_socket_->get_io_service().post(
            [this]() { this->do_async_send(); });
        return;
      }

      send_buffer_.resize(boost::asio::buffer_size(buffers));
      boost::asio::buffer_copy(boost::asio::buffer(send_buffer_), buffers);
    }

    internal_send_datagram_ =
        std::move(protocol_type::make_datagram(boost::asio::buffer(send_buffer_)));

    auto self = this->shared_from_this();
    AsyncSendDatagram(
        *p_internal_socket_, internal_send_datagram_,
        [self, this](const boost::system::error_code& ec, std::size_t length) {
          this->handle_sent(ec, length);
        });
  }

 private:
  p_internal_socket_type p_internal_socket_;
  boost::recursive_mutex closed_mutex_;
  bool closed_;

  boost::recursive_mutex receive_mutex_;
  bool receive_pending_;
  receive_datagram_type internal_receive_datagram_;
  receive_op_queue_type receive_op_queue_;

  boost::recursive_mutex send_mutex_;
  bool send_pending_;
  send_buffer_type send_buffer_;
  send_datagram_type internal_send_datagram_;
  send_op_queue_type send_op_queue_;
};

}  // interface_layer
}  // layer
}  // ssf

#endif  // SSF_LAYER_INTERFACE_LAYER_SPECIFIC_INTERFACE_SOCKET_H_
