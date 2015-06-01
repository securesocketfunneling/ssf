//
// fiber/stream_fiber.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2014-2015
//

#ifndef SSF_COMMON_BOOST_ASIO_FIBER_STREAM_FIBER_HPP
#define SSF_COMMON_BOOST_ASIO_FIBER_STREAM_FIBER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/basic_socket_acceptor.hpp>
#include <boost/asio/basic_socket_iostream.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/detail/socket_option.hpp>
#include <boost/asio/detail/socket_types.hpp>
#include <boost/asio/ip/basic_resolver.hpp>
#include <boost/asio/ip/basic_resolver_iterator.hpp>
#include <boost/asio/ip/basic_resolver_query.hpp>

#include <boost/asio/detail/push_options.hpp>

#include "common/boost/fiber/basic_endpoint.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"
#include "common/boost/fiber/stream_fiber_service.hpp"
#include "common/boost/fiber/fiber_acceptor_service.hpp"

namespace boost {
namespace asio {
namespace fiber {

/// Encapsulates the flags needed for stream fiber.
/**
* The boost::asio::fiber::stream_fiber class contains flags necessary for stream fiber.
*/
template <typename Socket>
class stream_fiber
{
public:
  /// Socket type behind fiber
  typedef Socket socket_type;

  /// Demultiplexer type
  typedef typename boost::asio::fiber::basic_fiber_demux<socket_type> demux_type;

  /// The type of a stream fiber endpoint.
  typedef basic_endpoint<stream_fiber> endpoint;

  /// Construct to represent the version 1 Fiber protocol.
  static stream_fiber v1()
  {
    return stream_fiber(BOOST_ASIO_OS_DEF(AF_INET));
  }

  /// Obtain an identifier for the type of the protocol.
  int type() const
  {
    return BOOST_ASIO_OS_DEF(SOCK_STREAM);
  }

  /// Obtain an identifier for the protocol.
  int protocol() const
  {
    return BOOST_ASIO_OS_DEF(IPPROTO_TCP);
  }

  /// Obtain an identifier for the protocol family.
  int family() const
  {
    return family_;
  }

  /// The stream fiber type.
  typedef boost::asio::basic_stream_socket<
    stream_fiber,
    boost::asio::fiber::stream_fiber_service<stream_fiber<Socket>>> socket;

  /// The stream fiber acceptor type.
  typedef boost::asio::basic_socket_acceptor<
    stream_fiber, boost::asio::fiber::fiber_acceptor_service<
      stream_fiber<Socket>>> acceptor;

  /// Compare two protocols for equality.
  friend bool operator==(const stream_fiber& p1, const stream_fiber& p2)
  {
    return p1.family_ == p2.family_;
  }

  /// Compare two protocols for inequality.
  friend bool operator!=(const stream_fiber& p1, const stream_fiber& p2)
  {
    return p1.family_ != p2.family_;
  }

private:
  // Construct with a specific family.
  explicit stream_fiber(int protocol_family)
    : family_(protocol_family)
  {
  }

  int family_;
};

} // namespace fiber
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif  // SSF_COMMON_BOOST_ASIO_FIBER_STREAM_FIBER_HPP
