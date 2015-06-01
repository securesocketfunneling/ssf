//
// fiber/detail/fiber_id.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2014-2015
//

#ifndef SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_FIBER_ID_HPP_
#define SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_FIBER_ID_HPP_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstdint>
#include <array>
#include <vector>

#include <boost/asio/buffer.hpp>

namespace boost {
namespace asio {
namespace fiber {
namespace detail {

/// Class handling the id field of the fiber header
class fiber_id
{
public:
  /// Type for a port
  typedef uint32_t port_type;

  /// Type for a remote fiber port
  typedef port_type remote_port_type;

  /// Type for a local fiber port
  typedef port_type local_port_type;

public:
  /// Constant used with the class
  enum { field_number = 2 };

public:
  /// Raw data structure to contain a fiber id
  struct raw_fiber_id
  {
    /// Local port field
    local_port_type local_port;

    /// Remote port field
    remote_port_type remote_port;
  };

  /// Get a raw id
  raw_fiber_id get_raw()
  {
    raw_fiber_id raw;

    raw.local_port = local_port_;
    raw.remote_port = remote_port_;

    return raw;
  }

public:
  /// Constructor setting only the remote port
  /**
  * @param remote_port The remote port to set in the id
  */
  explicit fiber_id(remote_port_type remote_port)
    : remote_port_(remote_port), local_port_(0)
  {
  }

  /// Constructor setting both ports
  /**
  * @param remote_port The remote port to set in the id
  * @param local_port The local port to set in the id
  */
  fiber_id(remote_port_type remote_port, local_port_type local_port)
    : remote_port_(remote_port), local_port_(local_port)
  {
  }

  /// Operator< to enable std::map support
  /**
  * @param fib_id The fiber id to be compared to
  */
  bool operator<(const fiber_id& fib_id) const
  {
    if (remote_port_ < fib_id.remote_port_)
    {
      return true;
    }
    else
    {
      return ((remote_port_ == fib_id.remote_port_) &&
              (local_port_ < fib_id.local_port_));
    }
  }

  /// Get the return id
  fiber_id returning_id() const { return fiber_id(local_port_, remote_port_); }

  /// Get the remote port
  remote_port_type remote_port() const { return remote_port_; }

  /// Get the local port
  local_port_type local_port() const { return local_port_; }

  /// Set the local port
  /**
  * @param src The local port to set in the id
  */
  void set_local_port(local_port_type src) { local_port_ = src; }

  /// Set the remote port
  /**
  * @param dst The remote port to set in the id
  */
  void set_remote_port(remote_port_type dst) { remote_port_ = dst; }

  /// Check if both port are set
  bool is_set() const { return (remote_port_ != 0) && (local_port_ != 0); }

  /// Get a buffer to receive a fiber ID
  std::array<boost::asio::mutable_buffer, field_number> buffer()
  {
    std::array<boost::asio::mutable_buffer, field_number> buf = {
      { boost::asio::buffer(&local_port_, sizeof(local_port_)),
      boost::asio::buffer(&remote_port_, sizeof(remote_port_)), } };
    return buf;
  }

  /// Get a buffer to send a fiber ID
  std::array<boost::asio::const_buffer, field_number> const_buffer()
  {
    std::array<boost::asio::const_buffer, field_number> buf = {
      { boost::asio::buffer(&local_port_, sizeof(local_port_)),
      boost::asio::buffer(&remote_port_, sizeof(remote_port_)), } };
    return buf;
  }

  /// Fill the given buffer with the fiber id fields
  /**
  * @param buffers The vector of buffers to fill
  */
  void fill_buffer(std::vector<boost::asio::mutable_buffer>& buffers)
  {
    buffers.push_back(boost::asio::buffer(&local_port_, sizeof(local_port_)));
    buffers.push_back(boost::asio::buffer(&remote_port_, sizeof(remote_port_)));
  }

  /// Fill the given buffer with the fiber id fields
  /**
  * @param buffers The vector of buffers to fill
  */
  void fill_const_buffer(std::vector<boost::asio::const_buffer>& buffers)
  {
    buffers.push_back(boost::asio::buffer(&local_port_, sizeof(local_port_)));
    buffers.push_back(boost::asio::buffer(&remote_port_, sizeof(remote_port_)));
  }

  /// Get the size of a fiber ID
  static uint16_t pod_size()
  {
    return sizeof(local_port_type)+sizeof(remote_port_type);
  }

private:
  remote_port_type remote_port_;
  local_port_type local_port_;
};

} // namespace detail
} // namespace fiber
} // namespace asio
} // namespace boost

#endif  // SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_FIBER_ID_HPP_
