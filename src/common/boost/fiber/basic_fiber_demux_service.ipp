//
// fiber/basic_fiber_demux_service.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2014-2015
//

#ifndef SSF_COMMON_BOOST_ASIO_FIBER_BASIC_FIBER_DEMUX_SERVICE_IMPL_IPP_
#define SSF_COMMON_BOOST_ASIO_FIBER_BASIC_FIBER_DEMUX_SERVICE_IMPL_IPP_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif  // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <ctime>
#include <functional>
#include <limits>
#include <mutex>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "common/boost/fiber/detail/basic_fiber_demux_impl.hpp"
#include "common/boost/fiber/detail/fiber_buffer.hpp"
#include "common/boost/fiber/detail/fiber_header.hpp"
#include "common/boost/fiber/detail/fiber_id.hpp"
#include "common/error/error.h"

namespace boost {
namespace asio {
namespace fiber {

template <typename S>
void basic_fiber_demux_service<S>::fiberize(implementation_type impl) {
  if (!impl) {
    SSF_LOG("demux", debug, "fiberizing NOK {}", ::error::broken_pipe);
    return;
  }

  SSF_LOG("demux", trace, "fiberizing");

  async_poll_packets(impl);
}

template <typename S>
void basic_fiber_demux_service<S>::bind(implementation_type impl,
                                        local_port_type local_port,
                                        fiber_impl_type fib_impl,
                                        boost::system::error_code& ec) {
  if (!impl) {
    SSF_LOG("demux", debug, "error bind NOK {}", ::error::broken_pipe);
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    return;
  }

  if (!local_port) {
    local_port = get_available_local_port(impl);
  }

  fib_impl->id.set_local_port(local_port);
  fiber_id id = fib_impl->id;

  fiber_id receiving_id = id.returning_id();

  {
    std::unique_lock<std::recursive_mutex> lock1(impl->bound_mutex);
    std::unique_lock<std::recursive_mutex> lock2(impl->used_ports_mutex);
    SSF_LOG("demux", trace, "try to bind fiber to {}:{}", id.local_port(),
            id.remote_port());
    if (receiving_id.remote_port() && !impl->bound.count(receiving_id)) {
      SSF_LOG("demux", trace, "bind OK");
      impl->bound[receiving_id] = fib_impl;
      impl->used_ports.insert(id.local_port());

      {
        std::unique_lock<std::recursive_mutex> lock_state(
            fib_impl->state_mutex);
        fib_impl->closed = false;
      }

      ec.assign(::error::success, ::error::get_ssf_category());
    } else {
      SSF_LOG("demux", debug, "bind NOK {}", ::error::device_or_resource_busy);
      ec.assign(::error::device_or_resource_busy, ::error::get_ssf_category());
    }
  }
}

template <typename S>
bool basic_fiber_demux_service<S>::is_bound(implementation_type impl,
                                            const fiber_id& id) {
  if (!impl) {
    SSF_LOG("demux", debug, "is_bound NOK {}", ::error::broken_pipe);
    return false;
  }

  std::unique_lock<std::recursive_mutex> lock(impl->bound_mutex);
  return !!impl->bound.count(id.returning_id());
}

template <typename S>
void basic_fiber_demux_service<S>::unbind(implementation_type impl,
                                          const fiber_id& id) {
  if (!impl) {
    SSF_LOG("demux", debug, "unbind NOK {}", ::error::broken_pipe);
    return;
  }

  std::unique_lock<std::recursive_mutex> lock1(impl->bound_mutex);
  std::unique_lock<std::recursive_mutex> lock2(impl->used_ports_mutex);
  SSF_LOG("demux", trace, "unbind {}, {}", id.local_port(), id.remote_port());

  impl->bound.erase(id.returning_id());
  impl->used_ports.erase(id.local_port());
}

template <typename S>
void basic_fiber_demux_service<S>::listen(implementation_type impl,
                                          local_port_type local_port,
                                          boost::system::error_code& ec) {
  if (!impl) {
    SSF_LOG("demux", debug, "[demux] listen NOK {}", ::error::broken_pipe);
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    return;
  }

  std::unique_lock<std::recursive_mutex> lock(impl->listening_mutex);

  if (!impl->listening.count(local_port) &&
      is_bound(impl, fiber_id(0, local_port))) {
    SSF_LOG("demux", trace, "fiber listening on {}", local_port);

    impl->listening.insert(local_port);

    ec.assign(::error::success, ::error::get_ssf_category());
  } else if (impl->listening.count(local_port)) {
    ec.assign(::error::device_or_resource_busy, ::error::get_ssf_category());
  } else {
    ec.assign(::error::protocol_error, ::error::get_ssf_category());
  }
}

template <typename S>
bool basic_fiber_demux_service<S>::is_listening(implementation_type impl,
                                                local_port_type local_port) {
  if (!impl) {
    SSF_LOG("demux", debug, "is_listening NOK {}", ::error::broken_pipe);
    return false;
  }

  std::unique_lock<std::recursive_mutex> lock(impl->listening_mutex);
  return !!impl->listening.count(local_port);
}

template <typename S>
void basic_fiber_demux_service<S>::stop_listening(implementation_type impl,
                                                  local_port_type local_port) {
  if (!impl) {
    SSF_LOG("demux", debug, "stop_listening NOK {}", ::error::broken_pipe);
    return;
  }

  std::unique_lock<std::recursive_mutex> lock1(impl->listening_mutex);
  std::unique_lock<std::recursive_mutex> lock2(impl->used_ports_mutex);
  SSF_LOG("demux", trace, "stopped listening on {}", local_port);

  impl->listening.erase(local_port);
  impl->used_ports.erase(local_port);
}

template <typename S>
void basic_fiber_demux_service<S>::async_poll_packets(
    implementation_type impl) {
  auto p_fiber_buff =
      std::make_shared<boost::asio::fiber::detail::fiber_buffer>();

  /////////////////// BEGIN HANDLER ///////////////////////////////
  auto dispatch_handler = [this, p_fiber_buff, impl](
      const boost::system::error_code& ec, std::size_t bytes_transferred) {
    std::unique_lock<std::recursive_mutex> lock(impl->closing_mutex);
    if (impl->closing) {
      return;
    }

    if (!ec) {
      this->dispatch_buffer(impl, p_fiber_buff);
      this->async_poll_packets(impl);
    } else {
      SSF_LOG("demux", debug,
              "error in dispatch handler {}: {} | {} bytes transferred",
              ec.value(), ec.message(), bytes_transferred);
      this->close(impl);
    }
  };
  //////////////////// END HANDLER ///////////////////////////////

  std::unique_lock<std::recursive_mutex> lock(impl->closing_mutex);
  if (!impl->closing) {
    async_read_fiber_buffer(impl->socket, *p_fiber_buff, dispatch_handler);
  }
}

template <typename S>
void basic_fiber_demux_service<S>::async_push_packets(
    implementation_type impl) {
  std::unique_lock<std::recursive_mutex> lock(impl->send_mutex);
  const auto& to_send_priority = impl->toSendPriority.front();

  auto handler = [this, impl, to_send_priority](
      const boost::system::error_code& ec, size_t transferred_bytes) {
    std::unique_lock<std::recursive_mutex> lock(impl->send_mutex);
    impl->toSendPriority.pop();
    impl->socket.get_io_service().post(
        std::bind(to_send_priority.handler, ec, transferred_bytes));
    if (!impl->toSendPriority.empty()) {
      impl->socket.get_io_service().post(std::bind(
          &basic_fiber_demux_service<S>::async_push_packets, this, impl));
    }
  };

  std::unique_lock<std::recursive_mutex> lock2(impl->closing_mutex);
  if (!impl->closing) {
    boost::asio::async_write(impl->socket, to_send_priority.buffer, handler);
  } else {
    impl->socket.get_io_service().post(std::bind(
        handler, boost::system::error_code(::error::connection_aborted,
                                           ::error::get_ssf_category()),
        0));
  }
}

template <typename S>
void basic_fiber_demux_service<S>::dispatch_buffer(
    implementation_type impl, p_fiber_buffer p_fiber_buff) {
  const auto& header = p_fiber_buff->header();

  const auto flags = header.flags();

  SSF_LOG("demux", trace, "dispatch {} {} {} {} {}", uint32_t(header.version()),
          header.id().remote_port(), header.id().local_port(), uint32_t(flags),
          header.data_size());

  switch (flags) {
    case kFlagPush:
      handle_push(impl, p_fiber_buff);
      break;
    case kFlagSyn:
      handle_syn(impl, p_fiber_buff);
      break;
    case kFlagReset:
      handle_rst(impl, p_fiber_buff);
      break;
    case kFlagAck:
      handle_ack(impl, p_fiber_buff);
      break;
    case kFlagDatagram:
      handle_dgr(impl, p_fiber_buff);
      break;
    default:
      break;
  }
}

template <typename S>
void basic_fiber_demux_service<S>::handle_dgr(implementation_type impl,
                                              p_fiber_buffer p_fiber_buff) {
  SSF_LOG("demux", trace, "handle dgr");
  const auto& full_id = p_fiber_buff->header().id();
  const auto& half_id = fiber_id(p_fiber_buff->header().id().remote_port(), 0);
  std::unique_lock<std::recursive_mutex> lock(impl->bound_mutex);

  if (impl->bound.count(full_id) != 0) {
    if (impl->bound[full_id]->accepts_dgr) {
      auto on_new_packet = impl->bound[full_id]->access_receive_dgr_handler();
      on_new_packet(p_fiber_buff->take_data(),
                    p_fiber_buff->header().id().local_port(),
                    p_fiber_buff->data_size());
    }
  } else if (impl->bound.count(half_id) != 0) {
    if (impl->bound[half_id]->accepts_dgr) {
      auto on_new_packet = impl->bound[half_id]->access_receive_dgr_handler();
      on_new_packet(p_fiber_buff->take_data(),
                    p_fiber_buff->header().id().local_port(),
                    p_fiber_buff->data_size());
    }
  }
}

template <typename S>
void basic_fiber_demux_service<S>::handle_push(implementation_type impl,
                                               p_fiber_buffer p_fiber_buff) {
  SSF_LOG("demux", trace, "handle push");
  const auto& header = p_fiber_buff->header();
  std::unique_lock<std::recursive_mutex> lock(impl->bound_mutex);

  if (impl->bound.count(header.id()) != 0) {
    auto on_new_packet = impl->bound[header.id()]->access_receive_handler();
    on_new_packet(p_fiber_buff->take_data(), p_fiber_buff->data_size());
  } else {
    async_send_rst(impl, header.id().returning_id(), []() {});
  }
}

template <typename S>
void basic_fiber_demux_service<S>::handle_ack(implementation_type impl,
                                              p_fiber_buffer p_fiber_buff) {
  const auto& header = p_fiber_buff->header();
  std::unique_lock<std::recursive_mutex> lock_bound(impl->bound_mutex);
  SSF_LOG("demux", trace, "handle ack");

  if (impl->bound.count(header.id())) {
    auto p_fib_impl = impl->bound[header.id()];
    p_fib_impl->toggle_out();

    std::unique_lock<std::recursive_mutex> lock_state(p_fib_impl->state_mutex);

    if (p_fib_impl->connecting) {
      p_fib_impl->set_connected();
      auto on_ack = p_fib_impl->access_connect_handler();
      on_ack(boost::system::error_code(::error::success,
                                       ::error::get_ssf_category()));
    }
  } else {
    async_send_rst(impl, header.id().returning_id(), []() {});
  }
}

template <typename S>
void basic_fiber_demux_service<S>::handle_syn(implementation_type impl,
                                              p_fiber_buffer p_fiber_buff) {
  SSF_LOG("demux", trace, "handle syn");
  const auto& header = p_fiber_buff->header();
  std::unique_lock<std::recursive_mutex> lock_listening(impl->listening_mutex);
  std::unique_lock<std::recursive_mutex> lock_bound(impl->bound_mutex);

  if (impl->listening.count(header.id().remote_port())) {
    auto on_new_fiber = impl->bound[fiber_id(header.id().remote_port())]
                            ->access_accept_handler();
    io_service_.post(std::bind(on_new_fiber, header.id().local_port()));
  } else {
    async_send_rst(impl, header.id().returning_id(), []() {});
  }
}

template <typename S>
void basic_fiber_demux_service<S>::handle_rst(implementation_type impl,
                                              p_fiber_buffer p_fiber_buff) {
  SSF_LOG("demux", trace, "handle rst");
  const auto& header = p_fiber_buff->header();
  auto returning_id = header.id().returning_id();
  std::unique_lock<std::recursive_mutex> lock_bound(impl->bound_mutex);
  if ((impl->bound).count(header.id())) {
    auto p_fib_impl = impl->bound[header.id()];
    auto on_close = p_fib_impl->access_close_handler();

    std::unique_lock<std::recursive_mutex> lock_state(p_fib_impl->state_mutex);

    if (p_fib_impl->connecting || p_fib_impl->connected) {
      if (p_fib_impl->connecting) {
        p_fib_impl->set_disconnected();
        auto on_connection = p_fib_impl->access_connect_handler();
        on_connection(boost::system::error_code(::error::connection_refused,
                                                ::error::get_ssf_category()));
      } else {
        p_fib_impl->set_disconnected();
        auto rst_sent = [this, impl, returning_id, p_fib_impl]() {
          this->unbind(impl, returning_id);
          p_fib_impl->access_close_handler()();
        };
        async_send_rst(impl, returning_id, std::move(rst_sent));
      }
    } else if (p_fib_impl->disconnecting) {
      p_fib_impl->set_disconnected();
      unbind(impl, returning_id);
      on_close();
    }
  }
}

template <typename S>
template <typename ConstBufferSequence, typename Handler>
void basic_fiber_demux_service<S>::async_send_push(implementation_type impl,
                                                   fiber_id id,
                                                   ConstBufferSequence& buffer,
                                                   Handler& handler) {
  std::unique_lock<std::recursive_mutex> lock1(impl->bound_mutex);

  if (impl->bound.count(id.returning_id())) {
    auto p_fiber_impl = impl->bound[id.returning_id()];
    if (p_fiber_impl->ready_out) {
      async_send(impl, id, kFlagPush, buffer, handler, p_fiber_impl->priority);
    } else {
      auto p_timer = std::make_shared<boost::asio::steady_timer>(io_service_);
      p_timer->expires_from_now(std::chrono::milliseconds(10));

      auto lambda = [handler,
                     p_timer](const boost::system::error_code&) mutable {
        handler(boost::system::error_code(), 0);
      };

      p_timer->async_wait(lambda);
    }
  } else {
    handler(boost::system::error_code(::error::protocol_error,
                                      ::error::get_ssf_category()),
            0);
  }
}

template <typename S>
template <typename ConstBufferSequence, typename Handler>
void basic_fiber_demux_service<S>::async_send_dgr(implementation_type impl,
                                                  remote_port_type remote_port,
                                                  fiber_impl_type fib_impl,
                                                  ConstBufferSequence& buffer,
                                                  Handler& handler) {
  std::unique_lock<std::recursive_mutex> lock1(impl->bound_mutex);

  if (fib_impl->id.local_port() == 0) {
    fib_impl->id.set_local_port(get_available_local_port(impl));
    boost::system::error_code ec;
    bind(impl, fib_impl->id.local_port(), fib_impl, ec);
    if (ec) {
      SSF_LOG("demux", debug, "error dgr {} {}", ec.message(), ec.value());
      io_service_.post(std::bind(handler, ec, 0));
      return;
    }
  }

  if (impl->bound.count(fib_impl->id.returning_id())) {
    if (fib_impl->ready_out) {
      async_send(impl, fiber_id(remote_port, fib_impl->id.local_port()),
                 kFlagDatagram, buffer, handler, fib_impl->priority);
    } else {
      auto p_timer = std::make_shared<boost::asio::steady_timer>(io_service_);
      p_timer->expires_from_now(std::chrono::milliseconds(10));

      auto lambda = [handler,
                     p_timer](const boost::system::error_code&) mutable {
        handler(boost::system::error_code(), 0);
      };

      p_timer->async_wait(lambda);
    }
  } else {
    io_service_.post(std::bind(
        handler, boost::system::error_code(::error::protocol_error,
                                           ::error::get_ssf_category()),
        0));
  }
}

template <typename S>
void basic_fiber_demux_service<S>::async_send_ack(implementation_type impl,
                                                  fiber_impl_type fib_impl,
                                                  accept_op* op) {
  fib_impl->toggle_in();

  if (op) {
    boost::system::error_code ec;
    bind(impl, fib_impl->id.local_port(), fib_impl, ec);
    fib_impl->set_connected();
    if (ec) {
      SSF_LOG("demux", debug, "error send ack {} {}", ec.message(), ec.value());
      op->complete(ec, 0);
    } else {
      auto handler = [=](const boost::system::error_code& ec, std::size_t) {
        if (ec) {
          SSF_LOG("demux", debug, "error send ack handler {}", ec.message());
        } else {
          SSF_LOG("demux", trace, "ack sent");
        }

        op->complete(ec, 0);
      };

      boost::asio::const_buffer pre_buffer;
      boost::asio::const_buffers_1 buffer(pre_buffer);
      async_send(impl, fib_impl->id, kFlagAck, buffer, handler, 0);
    }
  } else {
    boost::asio::const_buffer pre_buffer;
    boost::asio::const_buffers_1 buffer(pre_buffer);

    auto lambda = [](const boost::system::error_code&, std::size_t) {};
    async_send(impl, fib_impl->id, kFlagAck, buffer, lambda, 0);
  }
}

template <typename S>
void basic_fiber_demux_service<S>::async_send_syn(implementation_type impl,
                                                  fiber_id id) {
  std::unique_lock<std::recursive_mutex> lock_bound(impl->bound_mutex);
  // Bind reverse the id in bound map so reverse it...
  if ((impl->bound).count(id.returning_id())) {
    auto p_fib_impl = impl->bound[id.returning_id()];
    SSF_LOG("demux", trace, "async send syn");

    std::unique_lock<std::recursive_mutex> lock_state(p_fib_impl->state_mutex);

    if (!p_fib_impl->connecting) {
      p_fib_impl->set_connecting();
      auto handler = [impl, p_fib_impl](const boost::system::error_code& ec,
                                        std::size_t) {
        if (ec) {
          SSF_LOG("demux", debug, "syn error {}", ec.message());
          auto connection_failed = [p_fib_impl, ec]() {
            p_fib_impl->access_connect_handler()(ec);
          };
          impl->socket.get_io_service().post(connection_failed);
        } else {
          SSF_LOG("demux", trace, "syn sent");
        }
      };

      boost::asio::const_buffer pre_buffer;
      boost::asio::const_buffers_1 buffer(pre_buffer);
      async_send(impl, id, kFlagSyn, buffer, handler, 0);
    }
  }
}

template <typename S>
template <typename Handler>
void basic_fiber_demux_service<S>::async_send_rst(
    implementation_type impl, fiber_id id, const Handler& close_handler) {
  SSF_LOG("demux", trace, "async send rst");
  auto handler = [this, id, close_handler](const boost::system::error_code& ec,
                                           std::size_t) {
    if (ec) {
      SSF_LOG("demux", debug, "async send rst error {}: {}", ec.value(),
              ec.message());
    } else {
      SSF_LOG("demux", trace, "rst sent {} {}", id.local_port(),
              id.remote_port());
    }

    this->get_io_service().dispatch(close_handler);
  };
  boost::asio::const_buffer pre_buffer;
  boost::asio::const_buffers_1 buffer(pre_buffer);

  async_send(impl, id, kFlagReset, buffer, handler, 0);
}

template <typename S>
boost::asio::fiber::detail::fiber_id::local_port_type
basic_fiber_demux_service<S>::get_available_local_port(
    implementation_type impl) {
  local_port_type new_port = (1 << 17) + 1024, rand_port = 0;
  std::unique_lock<std::recursive_mutex> lock1(impl->used_ports_mutex);

  boost::random::uniform_int_distribution<uint32_t> dist(
      new_port, std::numeric_limits<uint32_t>::max());
  for (uint32_t i = 0; i < 100; ++i) {
    rand_port = dist(gen_);
    if (impl->used_ports.count(rand_port) == 0) {
      return rand_port;
    }
  }

  return 0;
}

template <typename S>
void basic_fiber_demux_service<S>::async_connect(
    implementation_type impl,
    boost::asio::fiber::detail::fiber_id::remote_port_type remote_port,
    fiber_impl_type fib_impl) {
  SSF_LOG("demux", trace, "async connect to remote port: {}", remote_port);

  fib_impl->id.set_remote_port(remote_port);

  boost::system::error_code ec;

  bind(impl, 0, fib_impl, ec);

  if (ec) {
    auto connection_failed = [=]() { fib_impl->access_connect_handler()(ec); };
    impl->socket.get_io_service().post(connection_failed);
  } else {
    async_send_syn(impl, fib_impl->id);
  }
}

template <typename S>
template <typename ConstBufferSequence, typename Handler>
void basic_fiber_demux_service<S>::async_send(
    implementation_type impl, fiber_id id,
    boost::asio::fiber::detail::fiber_header::flags_type flags,
    ConstBufferSequence& buffers, Handler handler, uint8_t priority) {
  auto buffers_size = boost::asio::buffer_size(buffers);

  if (buffers_size > impl->mtu) {
    if (flags & kFlagDatagram) {
      io_service_.post(std::bind(
          handler, boost::system::error_code(::error::message_too_long,
                                             ::error::get_ssf_category()),
          0));
      return;
    }
    buffers_size = impl->mtu;
  }

  auto new_buffers =
      get_partial_buffer_sequence<ConstBufferSequence>(buffers, buffers_size);

  fiber_header header(id, flags, (uint16_t)buffers_size);

  auto p_fiber_buffer = std::make_shared<fiber_buffer>();
  p_fiber_buffer->set_header(header);
  auto raw_buffer_to_send = p_fiber_buffer->const_buffer(new_buffers);

  auto do_user_handler = [p_fiber_buffer, handler](
      const boost::system::error_code& ec, size_t transferred_bytes) mutable {
    handler(ec, transferred_bytes - fiber_header::pod_size());
  };

  detail::extended_raw_fiber_buffer toSend(raw_buffer_to_send, do_user_handler,
                                           priority);

  auto do_push_packets = [this, toSend, impl]() {
    std::unique_lock<std::recursive_mutex> lock(impl->send_mutex);
    impl->toSendPriority.push(toSend);

    if (impl->toSendPriority.size() > 1) {
      return;
    }

    this->async_push_packets(impl);
  };

  auto& header_b = p_fiber_buffer->header();
  auto flags_b = header_b.flags();

  SSF_LOG("demux", trace, "sending {} {} {} {} {}",
          static_cast<uint32_t>(header_b.version()),
          header_b.id().remote_port(), header_b.id().local_port(),
          static_cast<uint32_t>(flags_b), header_b.data_size());

  impl->socket.get_io_service().post(do_push_packets);
}

template <typename S>
template <typename ConstBufferSequence>
std::vector<boost::asio::const_buffer>
basic_fiber_demux_service<S>::get_partial_buffer_sequence(
    ConstBufferSequence buffers, size_t length) {
  std::vector<boost::asio::const_buffer> new_buffers;

  for (auto& buffer : buffers) {
    auto buffer_size = boost::asio::buffer_size(buffer);
    if (length >= buffer_size) {
      new_buffers.push_back(buffer);
      length -= buffer_size;
    } else {
      const unsigned char* p1 =
          boost::asio::buffer_cast<const unsigned char*>(buffer);
      new_buffers.push_back(boost::asio::const_buffer(p1, length));
      length = 0;
      break;
    }
  }

  return new_buffers;
}

template <typename S>
void basic_fiber_demux_service<S>::close_fiber(implementation_type impl,
                                               fiber_impl_type fib_impl) {
  std::unique_lock<std::recursive_mutex> lock_state(fib_impl->state_mutex);
  auto on_close = fib_impl->access_close_handler();

  if (fib_impl->id.remote_port() == 0) {
    // fiber acceptor
    if (!fib_impl->closed) {
      stop_listening(impl, fib_impl->id.local_port());
      unbind(impl, fib_impl->id);
      impl->socket.get_io_service().post(on_close);
    }
  } else {
    // fiber
    if (!fib_impl->disconnecting && !fib_impl->disconnected) {
      if (fib_impl->connecting || fib_impl->connected) {
        fib_impl->set_disconnecting();
        async_send_rst(impl, fib_impl->id, [on_close] { on_close(); });
      }
    }
  }
}

template <typename S>
void basic_fiber_demux_service<S>::close_all_fibers(implementation_type impl) {
  std::map<fiber_id, fiber_impl_type> fibers;
  {
    std::unique_lock<std::recursive_mutex> lock1(impl->bound_mutex);
    fibers = impl->bound;
  }

  for (auto& fiber : fibers) {
    close_fiber(impl, fiber.second);
  }
}

template <typename S>
void basic_fiber_demux_service<S>::close(implementation_type impl) {
  if (impl) {
    std::unique_lock<std::recursive_mutex> lock(impl->closing_mutex);
    if (!impl->closing) {
      impl->closing = true;
      close_all_fibers(impl);
      auto close_handler = [impl]() {
        impl->close_handler();
        // reset close handler (circular dependency)
        impl->close_handler = []() {};
      };
      impl->socket.get_io_service().post(close_handler);
      // not enough: have to close socket...
      boost::system::error_code ec;
      impl->socket.close(ec);
    }
  }
}

template <typename S>
void basic_fiber_demux_service<S>::shutdown_service() {}

}  // namespace fiber
}  // namespace asio
}  // namespace boost

#endif  // SSF_COMMON_BOOST_ASIO_FIBER_BASIC_FIBER_DEMUX_SERVICE_IMPL_IPP_
