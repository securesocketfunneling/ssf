//
// fiber/detail/basic_fiber_demux_impl.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2014-2015
//

#ifndef SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_BASIC_FIBER_DEMUX_IMPL_HPP_
#define SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_BASIC_FIBER_DEMUX_IMPL_HPP_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif  // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <set>

#include "common/boost/fiber/detail/fiber_id.hpp"

namespace boost {
namespace asio {
namespace fiber {
namespace detail {

template <typename StreamSocket>
class basic_fiber_impl;

/// Class used to handle QoS in fiber sendings
template <typename Buffer, typename Handler>
struct extended_buffer {
  extended_buffer(const Buffer& b, const Handler& h, uint8_t p)
      : buffer(b), handler(h), priority(p) {}

  bool operator<(const extended_buffer& rhs) const {
    return priority < rhs.priority;
  }

  Buffer buffer;
  Handler handler;
  uint8_t priority;
};

typedef extended_buffer<std::vector<boost::asio::const_buffer>,
                        std::function<void(const boost::system::error_code&,
                                           size_t)>> extended_raw_fiber_buffer;

/// Implementation class of the fiber demultiplexer
template <typename StreamSocket>
class basic_fiber_demux_impl : public std::enable_shared_from_this<
                                   basic_fiber_demux_impl<StreamSocket>> {
 private:
  typedef boost::asio::fiber::detail::basic_fiber_impl<StreamSocket>
      fiber_impl_deref_type;
  typedef std::shared_ptr<fiber_impl_deref_type> fiber_impl_type;
  typedef std::map<fiber_id, fiber_impl_type> bind_map;
  typedef std::set<fiber_id::local_port_type> listen_set;
  typedef std::set<fiber_id::local_port_type> in_use_port_set;
  typedef std::shared_ptr<basic_fiber_demux_impl<StreamSocket>> p_impl;

  typedef std::function<void()> close_handler_type;

 private:
  basic_fiber_demux_impl(StreamSocket s, close_handler_type close, size_t a_mtu)
      : bound(),
        listening(),
        used_ports(),
        socket(std::move(s)),
        closing(false),
        mtu(a_mtu),
        close_handler(close) {}

 public:
  ~basic_fiber_demux_impl() {}

 public:
  static p_impl create(StreamSocket s, close_handler_type close, size_t mtu) {
    return p_impl(
        new basic_fiber_demux_impl<StreamSocket>(std::move(s), close, mtu));
  }

  void set_socket(StreamSocket s) { socket = std::move(s); }

  // Store the bound fibers
  std::recursive_mutex bound_mutex;
  bind_map bound;

  /// Store the ports on which fibers are listening
  std::recursive_mutex listening_mutex;
  listen_set listening;

  /// Store all currently used ports
  std::recursive_mutex used_ports_mutex;
  in_use_port_set used_ports;

  StreamSocket socket;

  std::recursive_mutex closing_mutex;
  bool closing;

  std::recursive_mutex send_mutex;

  /// maximum size of the payload of one packet
  size_t mtu;

  close_handler_type close_handler;

  // std::priority_queue<extended_raw_fiber_buffer> toSendPriority;
  std::queue<extended_raw_fiber_buffer> toSendPriority;
};

}  // namespace detail
}  // namespace fiber
}  // namespace asio
}  // namespace boost

#endif  // SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_BASIC_FIBER_DEMUX_IMPL_HPP_
