//
// fiber/datagram_fiber.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2014-2015
//

#ifndef SSF_COMMON_BOOST_ASIO_FIBER_DATAGRAM_FIBER_HPP
#define SSF_COMMON_BOOST_ASIO_FIBER_DATAGRAM_FIBER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/basic_datagram_socket.hpp>
#include <boost/asio/detail/socket_types.hpp>

#include <boost/asio/detail/push_options.hpp>

#include "common/boost/fiber/basic_endpoint.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"
#include "common/boost/fiber/datagram_fiber_service.hpp"

namespace boost {
namespace asio {
namespace fiber {

/// Encapsulates the flags needed for datagram fiber.
/**
  * The boost::asio::fiber::datagram_fiber class contains flags necessary for datagram fiber.
  */
template <typename Socket>
class datagram_fiber
{
public:
  /// Socket type behind fiber
  typedef Socket socket_type;
  
  /// Demultiplexer type
  typedef typename boost::asio::fiber::basic_fiber_demux<socket_type> demux_type;

  /// The type of a datagram fiber endpoint.
  typedef basic_endpoint<datagram_fiber> endpoint;

  /// Construct to represent the version 1 Fiber protocol.
  static datagram_fiber v1()
  {
    return datagram_fiber(BOOST_ASIO_OS_DEF(AF_INET));
  }

  /// Obtain an identifier for the type of the protocol.
  int type() const
  {
    return BOOST_ASIO_OS_DEF(SOCK_DGRAM);
  }

  /// Obtain an identifier for the protocol.
  int protocol() const
  {
    return BOOST_ASIO_OS_DEF(IPPROTO_UDP);
  }

  /// Obtain an identifier for the protocol family.
  int family() const
  {
    return family_;
  }

  /// The datagram fiber type.
  typedef boost::asio::basic_datagram_socket<datagram_fiber,
    boost::asio::fiber::datagram_fiber_service<datagram_fiber<Socket>>> socket;

  /// Compare two protocols for equality.
  friend bool operator==(const datagram_fiber& p1, const datagram_fiber& p2)
  {
    return p1.family_ == p2.family_;
  }

  /// Compare two protocols for inequality.
  friend bool operator!=(const datagram_fiber& p1, const datagram_fiber& p2)
  {
    return p1.family_ != p2.family_;
  }

private:
  // Construct with a specific family.
  explicit datagram_fiber(int protocol_family)
    : family_(protocol_family)
  {
  }

  int family_;
};

} // namespace fiber
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif  // SSF_COMMON_BOOST_ASIO_FIBER_DATAGRAM_FIBER_HPP
