//
// fiber/detail/basic_fiber_impl.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2014-2015
//

#ifndef SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_BASIC_FIBER_IMPL_HPP_
#define SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_BASIC_FIBER_IMPL_HPP_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif  // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <functional>
#include <memory>
#include <queue>

#include <ssf/log/log.h>

#include "common/error/error.h"
#include "common/boost/fiber/detail/fiber_id.hpp"
#include "common/boost/fiber/detail/fiber_header.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"
#include "common/boost/fiber/detail/io_fiber_read_op.hpp"
#include "common/boost/fiber/detail/io_fiber_dgr_read_op.hpp"
#include "common/boost/fiber/detail/io_fiber_accept_op.hpp"

namespace boost {
namespace asio {
namespace fiber {
namespace detail {

template <class T>
struct make_asio_queue {
  typedef boost::asio::detail::op_queue<T> type;
  typedef T* value_type;
};

template <class T>
struct make_queue {
  typedef std::queue<T> type;
  typedef T value_type;
};

template <typename StreamSocket>
class basic_fiber_impl
    : public std::enable_shared_from_this<basic_fiber_impl<StreamSocket>> {
 private:
  /// Type for a local fiber port
  typedef boost::asio::fiber::detail::fiber_id::local_port_type local_port_type;

  /// Type for a remote fiber port
  typedef boost::asio::fiber::detail::fiber_id::remote_port_type
      remote_port_type;

  /// Type for a pointer to a fiber impl
  typedef std::shared_ptr<basic_fiber_impl<StreamSocket>> p_impl;

  /// Type for an object storing the user read request
  typedef boost::asio::fiber::detail::basic_pending_read_operation read_op;

  /// Type for an object storing the user read request
  typedef boost::asio::fiber::detail::basic_pending_dgr_read_operation
      dgr_read_op;

  /// Type for an object storing the user accept request
  typedef boost::asio::fiber::detail::basic_pending_accept_operation<
      StreamSocket> accept_op;

  /// Type for the fiber demultiplexer
  typedef boost::asio::fiber::basic_fiber_demux<StreamSocket> fiber_demux_type;

  /// Type for the queue storing the pending read requests
  typedef make_asio_queue<read_op>::type read_op_queue_type;

  /// Type for the queue storing the pending read requests for datagrams
  typedef make_asio_queue<dgr_read_op>::type read_dgr_op_queue_type;

  /// Type for the structure used to store the data received
  typedef boost::asio::streambuf data_queue_type;

  typedef make_queue<std::vector<uint8_t>>::type dgr_data_queue_type;

  /// Type for the queue storing the pending accept requests
  typedef typename make_asio_queue<accept_op>::type accept_op_queue_type;

  /// Type for to store remote fiber ports (for accepting or datagrams)
  typedef make_queue<remote_port_type>::type remote_port_queue_type;

  /// Type of the handler used when accepting a new fiber
  typedef std::function<void(local_port_type)> accept_handler_type;

  /// Type of the handler used when connecting a new fiber
  typedef std::function<void(boost::system::error_code)> connect_handler_type;

  /// Type of the handler provided by the user when connecting a new fiber
  typedef std::function<void(const boost::system::error_code&)>
      connect_user_handler_type;

  /// Type of the handler used when receiving a new packet
  typedef std::function<void(std::vector<uint8_t>&&, std::size_t)>
      receive_handler_type;

  /// Type of the handler used when receiving a new datagram
  typedef std::function<void(std::vector<uint8_t>&&,
                             remote_port_type remote_port, std::size_t)>
      receive_dgr_handler_type;

  /// Type of the handler used when closing a fiber
  typedef std::function<void()> close_handler_type;

  /// Type of the handler provided by the user when closing a fiber
  typedef std::function<void(const boost::system::error_code&)>
      close_user_handler_type;

  /// Type of the handler used when an unknown error occurs
  typedef std::function<void(boost::system::error_code)> error_handler_type;

 private:
  /// Constructor for a fiber implementation object
  /**
  * @param f_demux The demultiplexer used for this fiber
  * @param remote_port The remote port to chich the fiber is to be bound
  * @param prio (Unused) The priority for this fiber on the demultiplexer
  * @param dgr The fiber accepts datagrams
  */
  basic_fiber_impl(fiber_demux_type* p_f_demux, remote_port_type remote_port,
                   uint8_t prio, bool dgr)
      : id(remote_port),
        p_fib_demux(p_f_demux),
        ready_in(true),
        ready_out(true),
        priority(prio),
        state_mutex(),
        closed(true),
        connecting(false),
        connected(false),
        disconnecting(false),
        disconnected(true),
        read_op_queue_mutex(),
        read_op_queue(),
        data_queue_mutex(),
        data_queue(),
        dgr_data_queue_(),
        accept_op_queue_mutex(),
        accept_op_queue(),
        port_queue_mutex(),
        port_queue(),
        accepts_dgr(dgr) {}

  basic_fiber_impl()
      : id(0),
        p_fib_demux(nullptr),
        ready_in(true),
        ready_out(true),
        priority(0),
        state_mutex(),
        closed(true),
        connecting(false),
        connected(false),
        disconnecting(false),
        disconnected(true),
        read_op_queue_mutex(),
        read_op_queue(),
        data_queue_mutex(),
        data_queue(),
        dgr_data_queue_(),
        accept_op_queue_mutex(),
        accept_op_queue(),
        port_queue_mutex(),
        port_queue(),
        accepts_dgr() {}

 public:
  /// Destructor
  ~basic_fiber_impl() {}

 public:
  /// Initialize the fiber impl by setting all its handler
  void init() {
    accept_handler = [this](remote_port_type remote_port) {
      {
        boost::recursive_mutex::scoped_lock lock(this->port_queue_mutex);
        this->port_queue.push(remote_port);
      }
      this->a_queues_handler();
    };

    connect_handler = [this](boost::system::error_code ec) {
      this->set_opened();

      this->init_connect_in_out();

      this->connect_user_handler(ec);
    };

    receive_handler =
        [this](std::vector<uint8_t>&& data, std::size_t bytes_transfered) {
          {
            boost::recursive_mutex::scoped_lock lock(this->data_queue_mutex);
            boost::asio::streambuf::mutable_buffers_type buffers =
                this->data_queue.prepare(bytes_transfered);

            boost::asio::buffer_copy(buffers, boost::asio::buffer(data));
            this->data_queue.commit(bytes_transfered);
          }
          this->r_queues_handler();
        };

    receive_dgr_handler =
        [this](std::vector<uint8_t>&& data, remote_port_type remote_port,
               std::size_t bytes_transfered) {
          {
            boost::recursive_mutex::scoped_lock lock1(this->data_queue_mutex);
            boost::recursive_mutex::scoped_lock lock2(this->port_queue_mutex);
            this->port_queue.push(remote_port);
            data.resize(bytes_transfered);
            this->dgr_data_queue_.push(data);
          }
          this->r_dgr_queues_handler();
        };

    close_handler = [this]() {
      boost::system::error_code ec(::error::connection_reset,
                                   ::error::get_ssf_category());
      cancel_operations(ec);

      SSF_LOG(kLogTrace) << "fiber impl: close handler "
                         << this->id.remote_port() << ":"
                         << this->id.local_port();

      this->set_closed();
    };

    error_handler = [](boost::system::error_code) {};
  }

  /// Accessor for the accept handler
  accept_handler_type access_accept_handler() {
    auto self = this->shared_from_this();
    auto lambda = [self, this](remote_port_type remote_port) {
      this->accept_handler(remote_port);
    };
    return lambda;
  }

  /// Accessor for the connect handler
  connect_handler_type access_connect_handler() {
    auto self = this->shared_from_this();
    auto lambda = [self, this](boost::system::error_code ec) {
      this->connect_handler(ec);
    };
    return lambda;
  }

  /// Accessor for the receive handler
  receive_handler_type access_receive_handler() {
    auto self = this->shared_from_this();
    auto lambda = [self, this](std::vector<uint8_t>&& data,
                               std::size_t bytes_transfered) {
      this->receive_handler(std::move(data), bytes_transfered);
    };
    return lambda;
  }

  /// Accessor for the receive handler for datagrams
  receive_dgr_handler_type access_receive_dgr_handler() {
    auto self = this->shared_from_this();
    auto lambda = [self, this](std::vector<uint8_t>&& data,
                               remote_port_type remote_port,
                               std::size_t bytes_transfered) {
      this->receive_dgr_handler(std::move(data), remote_port, bytes_transfered);
    };
    return lambda;
  }

  /// Accessor for the close handler
  close_handler_type access_close_handler() {
    auto self = this->shared_from_this();
    auto lambda = [self, this]() { this->close_handler(); };
    return lambda;
  }

  /// Accessor for the error handler
  error_handler_type access_error_handler() {
    auto self = this->shared_from_this();
    auto lambda = [self, this]() { this->error_handler(); };
    return lambda;
  }

  /// Create a new shared pointer to a fiber impl
  /**
  * @param f_demux The demultiplexer used for this fiber
  * @param remote_port The remote port to chich the fiber is to be bound
  * @param prio (Unused) The priority for this fiber on the demultiplexer
  * @param dgr The fiber accepts datagrams
  */
  static p_impl create(fiber_demux_type* p_f_demux,
                       remote_port_type remote_port, uint8_t prio = 0,
                       bool dgr = false) {
    p_impl res = p_impl(
        new basic_fiber_impl<StreamSocket>(p_f_demux, remote_port, prio, dgr));
    res->init();

    return res;
  }

  /// Create a new shared pointer to a fiber impl
  static p_impl create() {
    p_impl res = p_impl(new basic_fiber_impl<StreamSocket>());
    res->init();

    return res;
  }

  /// Handle the accept operations
  /**
  * @param ec The error code corresponding to the previous call to this function
  */
  void a_queues_handler(
      boost::system::error_code ec = boost::system::error_code()) {
    boost::recursive_mutex::scoped_lock lock1(accept_op_queue_mutex);
    boost::recursive_mutex::scoped_lock lock2(port_queue_mutex);

    if (!ec) {
      if (!accept_op_queue.empty() && !port_queue.empty()) {
        auto remote_port = port_queue.front();
        port_queue.pop();
        auto op = accept_op_queue.front();
        accept_op_queue.pop();
        op->set_remote_port(remote_port);

        op->get_p_fib()->init_accept_in_out();

        p_fib_demux->async_send_ack(op->get_p_fib(), op);

        SSF_LOG(kLogDebug) << "fiber impl: new connection from remote port: "
                           << remote_port;

        p_fib_demux->get_io_service().dispatch(boost::bind(
            &basic_fiber_impl::a_queues_handler, this->shared_from_this(), ec));
      }
    } else {
      if (!accept_op_queue.empty()) {
        auto op = accept_op_queue.front();
        accept_op_queue.pop();

        op->complete(ec, 0);
        a_queues_handler(ec);
      }
    }
  }

  /// Handle the read operations
  /**
  * @param ec The error code corresponding to the previous call to this function
  */
  void r_queues_handler(
      boost::system::error_code ec = boost::system::error_code()) {
    boost::recursive_mutex::scoped_lock lock1(read_op_queue_mutex);
    boost::recursive_mutex::scoped_lock lock2(data_queue_mutex);

    {
      boost::recursive_mutex::scoped_lock lock(in_mutex);
      if (((data_queue.size() > 60 * 1024 * 1024) && ready_in) ||
          ((data_queue.size() < 40 * 1024 * 1024) && !ready_in)) {
        p_fib_demux->async_send_ack(this->shared_from_this(), nullptr);
      }
    }

    SSF_LOG(kLogTrace) << "fiber impl: queue empty : " << read_op_queue.empty()
                       << " | queue size " << data_queue.size() << " | ec "
                       << ec.value();
    if (!ec) {
      if (!read_op_queue.empty() && data_queue.size()) {
        auto op = read_op_queue.front();
        read_op_queue.pop();

        size_t copied = op->fill_buffers(data_queue);

        auto do_complete =
            [=]() { op->complete(boost::system::error_code(), copied); };
        p_fib_demux->get_io_service().post(do_complete);

        p_fib_demux->get_io_service().dispatch(boost::bind(
            &basic_fiber_impl::r_queues_handler, this->shared_from_this(), ec));
      }
    } else {
      if (!read_op_queue.empty()) {
        auto op = read_op_queue.front();
        read_op_queue.pop();
        op->complete(ec, 0);
        r_queues_handler(ec);
      }
    }
  }

  /// Handle the read operations for datagrams
  /**
  * @param ec The error code corresponding to the previous call to this function
  */
  void r_dgr_queues_handler(
      boost::system::error_code ec = boost::system::error_code()) {
    boost::recursive_mutex::scoped_lock lock1(read_op_queue_mutex);
    boost::recursive_mutex::scoped_lock lock2(data_queue_mutex);
    boost::recursive_mutex::scoped_lock lock3(port_queue_mutex);
    SSF_LOG(kLogTrace) << "fiber impl: queue empty: " << read_op_queue.empty()
                       << " | port queue empty : " << port_queue.empty()
                       << " | queue size " << data_queue.size()
                       << " | dgr queue size " << dgr_data_queue_.size()
                       << " | ec " << ec.value();
    if (!ec) {
      if (!read_dgr_op_queue.empty() && !port_queue.empty() &&
          !dgr_data_queue_.empty()) {
        auto op = read_dgr_op_queue.front();
        read_dgr_op_queue.pop();

        auto remote_port = port_queue.front();
        port_queue.pop();

        auto data = dgr_data_queue_.front();
        dgr_data_queue_.pop();

        size_t copied = op->fill_buffers(data);

        op->set_remote_port(remote_port);

        auto do_complete =
            [=]() { op->complete(boost::system::error_code(), copied); };
        p_fib_demux->get_io_service().post(do_complete);

        p_fib_demux->get_io_service().dispatch(
            boost::bind(&basic_fiber_impl::r_dgr_queues_handler,
                        this->shared_from_this(), ec));
      }
    } else {
      if (!read_op_queue.empty()) {
        auto op = read_op_queue.front();
        read_op_queue.pop();

        auto do_complete = [=]() { op->complete(ec, 0); };
        p_fib_demux->get_io_service().post(do_complete);

        p_fib_demux->get_io_service().dispatch(
            boost::bind(&basic_fiber_impl::r_dgr_queues_handler,
                        this->shared_from_this(), ec));
      }
    }
  }

  /// Cancel all pending operations immediatly
  /**
  * @param ec The error code that will be given to the pending operations
  */
  void cancel_operations(
      boost::system::error_code ec = boost::system::error_code(
          ::error::interrupted, ::error::get_ssf_category())) {
    r_queues_handler(ec);
    a_queues_handler(ec);
  }

  /// Make the fiber able to send and unable to receive
  void init_accept_in_out() {
    boost::recursive_mutex::scoped_lock lock1(in_mutex);
    boost::recursive_mutex::scoped_lock lock2(out_mutex);

    ready_in = false;
    ready_out = true;
  }

  /// Make the fiber able to send and receive
  void init_connect_in_out() {
    boost::recursive_mutex::scoped_lock lock1(in_mutex);
    boost::recursive_mutex::scoped_lock lock2(out_mutex);

    ready_in = true;
    ready_out = true;
  }

  /// Toggle the fiber ability to receive
  void toggle_in() {
    boost::recursive_mutex::scoped_lock lock(in_mutex);
    ready_in = !ready_in;
  }

  /// Toggle the fiber ability to send
  void toggle_out() {
    boost::recursive_mutex::scoped_lock lock(out_mutex);
    ready_out = !ready_out;
  }

  void set_opened() {
    boost::recursive_mutex::scoped_lock lock_state(state_mutex);
    closed = false;
  }

  void set_closed() {
    boost::recursive_mutex::scoped_lock lock_state(state_mutex);
    closed = true;
  }

  /// Set implementation in connecting state
  void set_connecting() {
    boost::recursive_mutex::scoped_lock lock_state(state_mutex);
    connecting = true;
    connected = false;
    disconnecting = false;
    disconnected = false;
  }

  /// Set implementation in connected state
  void set_connected() {
    boost::recursive_mutex::scoped_lock lock_state(state_mutex);
    connecting = false;
    connected = true;
    closed = false;
    disconnecting = false;
    disconnected = false;
  }

  /// Set implementation in disconnecting state
  void set_disconnecting() {
    boost::recursive_mutex::scoped_lock lock_state(state_mutex);
    connecting = false;
    connected = false;
    disconnecting = true;
    disconnected = false;
  }

  /// Set implementation in disconnected state
  void set_disconnected() {
    boost::recursive_mutex::scoped_lock lock_state(state_mutex);
    connecting = false;
    connected = false;
    closed = true;
    disconnecting = false;
    disconnected = true;
  }

  fiber_id id;

  fiber_demux_type* p_fib_demux;

  boost::recursive_mutex in_mutex;
  boost::recursive_mutex out_mutex;
  bool ready_in;
  bool ready_out;

  uint8_t priority;

  boost::recursive_mutex state_mutex;
  // States of the fiber
  bool closed;
  bool connecting;
  bool connected;
  bool disconnecting;
  bool disconnected;

  boost::recursive_mutex read_op_queue_mutex;

  /// Store the pending read requests
  read_op_queue_type read_op_queue;

  boost::recursive_mutex read_dgr_op_queue_mutex;

  /// Store the pending read requests for datagrams
  read_dgr_op_queue_type read_dgr_op_queue;

  boost::recursive_mutex data_queue_mutex;

  /// Store the received data
  data_queue_type data_queue;

  /// Store the dgr received data
  dgr_data_queue_type dgr_data_queue_;

  boost::recursive_mutex accept_op_queue_mutex;

  /// Store the pending accept operation
  accept_op_queue_type accept_op_queue;

  boost::recursive_mutex port_queue_mutex;

  /// Store the connecting remote port
  remote_port_queue_type port_queue;

  /// Connect user handler
  connect_user_handler_type connect_user_handler;

  /// Fiber accepts datagram
  bool accepts_dgr;

 private:
  accept_handler_type accept_handler;
  connect_handler_type connect_handler;
  receive_handler_type receive_handler;
  receive_dgr_handler_type receive_dgr_handler;
  close_handler_type close_handler;
  error_handler_type error_handler;
};

}  // namespace detail
}  // namespace fiber
}  // namespace asio
}  // namespace boost

#endif  // SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_BASIC_FIBER_IMPL_HPP_
