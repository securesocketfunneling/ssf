//
// fiber/basic_endpoint.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2014-2015
//

#ifndef SSF_COMMON_BOOST_ASIO_FIBER_BASIC_ENDPOINT_HPP_
#define SSF_COMMON_BOOST_ASIO_FIBER_BASIC_ENDPOINT_HPP_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif  // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "common/boost/fiber/detail/fiber_id.hpp"

namespace boost {
namespace asio {
namespace fiber {

/// basic_endpoint class for fiber
/**
* An endpoint is composed of a demultiplexer and a remote fiber port
*
* @tparam Demux The type of the demultiplexer user
*/
template <typename Protocol>
class basic_endpoint {
 private:
  /// Type of the demultiplexer
  typedef typename Protocol::demux_type demux_type;
  /// Type for the remote fiber port
  typedef boost::asio::fiber::detail::fiber_id::port_type port_type;

 public:
  basic_endpoint() : p_demux_(nullptr), port_(0) {}

  /// Constructor
  /**
  * @param protocol Protocol
  * @param demux The demultiplexer on which to bind the endpoint
  */
  basic_endpoint(Protocol protocol, demux_type& demux)
      : p_demux_(&demux), port_(0) {}

  /// Constructor
  /**
  * @param demux The demultiplexer on which to bind the endpoint
  * @param port The port on which to bind the endpoint
  */
  basic_endpoint(demux_type& demux, port_type port)
      : p_demux_(&demux), port_(port) {}

  bool operator<(const basic_endpoint& rhs) const { return port_ < rhs.port_; }

  /// Constructor
  /**
  * @param protocol Protocol
  * @param demux The demultiplexer on which to bind the endpoint
  * @param port The port on which to bind the endpoint
  */
  basic_endpoint(Protocol protocol, demux_type& demux, port_type port)
      : family_(protocol.family()), p_demux_(&demux), port_(port) {}

  basic_endpoint& operator=(const basic_endpoint& rhs) {
    family_ = rhs.family_;
    p_demux_ = rhs.p_demux_;
    port_ = rhs.port_;

    return *this;
  }

  /// The protocol associated with the endpoint.
  Protocol protocol() const { return Protocol::v1(); }

  /// Get the demultiplexer
  demux_type& demux() const { return *p_demux_; }

  /// Get the port
  port_type port() const { return port_; }

  /// Get the port
  port_type& port() { return port_; }

 private:
  int family_;
  demux_type* p_demux_;
  port_type port_;
};

}  // namespace fiber
}  // namespace asio
}  // namespace boost

#endif  // SSF_COMMON_BOOST_ASIO_FIBER_BASIC_ENDPOINT_HPP_
