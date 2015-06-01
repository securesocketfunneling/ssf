//
// fiber/detail/io_operation.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2014-2015
//

#ifndef SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_IO_OPERATION_HPP_
#define SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_IO_OPERATION_HPP_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/handler_tracking.hpp>
#include <boost/asio/detail/op_queue.hpp>
#include <boost/system/error_code.hpp>
#include <boost/asio/streambuf.hpp>

#include <vector>

#include "common/boost/fiber/detail/fiber_id.hpp"

namespace boost {
namespace asio {
namespace fiber {
namespace detail {

template <typename StreamSocket>
class basic_fiber_impl;

/// Base class for pending io operations
class basic_pending_io_operation BOOST_ASIO_INHERIT_TRACKED_HANDLER
{
public:
  /// Function called on completion
  /**
  * @param ec The error code resulting of the completed operation
  * @param bytes_transferred The number of bytes read or written
  */
  void complete(const boost::system::error_code& ec,
                std::size_t bytes_transferred)
  {
    auto destroy = false;
    func_(this, destroy, ec, bytes_transferred);
  }

  void destroy()
  {
    auto destroy = true;
    func_(this, destroy, boost::system::error_code(), 0);
  }

protected:
  typedef void(*func_type)(basic_pending_io_operation*, bool,
                            const boost::system::error_code& ec, std::size_t);

  basic_pending_io_operation(func_type func) : next_(0), func_(func) {}

  ~basic_pending_io_operation() {}

  friend class boost::asio::detail::op_queue_access;
  basic_pending_io_operation* next_;
  func_type func_;
};

//-----------------------------------------------------------------------------

/// Base class for pending fiber accept operations
template <typename StreamSocket>
class basic_pending_accept_operation : public basic_pending_io_operation
{
private:
  typedef boost::asio::fiber::detail::basic_fiber_impl<StreamSocket> fiber_deref_impl;
  typedef std::shared_ptr<fiber_deref_impl> fiber_impl;

protected:
  /// Constructor
  /**
  * @param func The completion handler
  * @param p_fib The implementation of the accepted fiber
  * @param p_fib_id The id of the accepted fiber
  */
  basic_pending_accept_operation(basic_pending_io_operation::func_type func,
                                  fiber_impl p_fib, fiber_id* p_fib_id)
                                  : basic_pending_io_operation(func),
                                    p_fib_(p_fib),
                                    p_id_(p_fib_id)
  {
  }

public:
  /// Set the id of the accepted fiber
  /**
  * @param remote_port The remote port to set in the id
  */
  void set_remote_port(fiber_id::remote_port_type remote_port)
  {
    p_id_->set_remote_port(remote_port);
  }

  /// Get the accepted fiber implementation
  fiber_impl get_p_fib() { return p_fib_; }

private:
  fiber_impl p_fib_;
  fiber_id* p_id_;
};

//-----------------------------------------------------------------------------

/// Base class for pending stream read operations
class basic_pending_read_operation : public basic_pending_io_operation
{
private:
  typedef size_t(*fill_buffer_func_type)(basic_pending_io_operation*,
                                          boost::asio::streambuf&);

protected:
  /// Constructor
  /**
  * @param func The completion handler
  * @param fill_buffer_func The function to fill the buffer with read data
  * @param p_remote_port (Optional) The remote port from where the data was
  * received
  */
  basic_pending_read_operation(
    basic_pending_io_operation::func_type func,
    fill_buffer_func_type fill_buffer_func,
    fiber_id::remote_port_type* p_remote_port = nullptr)
    : basic_pending_io_operation(func),
    fill_buffer_func_(fill_buffer_func),
    p_remote_port_(p_remote_port)
  {
  }

public:
  /// Function called to fill the buffer with read data
  /**
  * @param buf The buffer where to store the read data
  */
  size_t fill_buffers(boost::asio::streambuf& buf)
  {
    if (fill_buffer_func_)
    {
      return fill_buffer_func_(this, buf);
    }
    else
    {
      return 0;
    }
  }

  /// Set the remote port
  /**
  * @param remote_port The remote port value to set
  */
  bool set_remote_port(const fiber_id::remote_port_type& remote_port)
  {
    if (p_remote_port_)
    {
      *p_remote_port_ = remote_port;
      return true;
    }
    else
    {
      return false;
    }
  }

private:
  fill_buffer_func_type fill_buffer_func_;
  fiber_id::remote_port_type* p_remote_port_;
};

/// Base class for pending dgr read operations
class basic_pending_dgr_read_operation : public basic_pending_io_operation
{
private:
  typedef size_t(*fill_dgr_buffer_func_type)(basic_pending_io_operation*,
                                             const std::vector<uint8_t>&);

protected:
  /// Constructor
  /**
  * @param func The completion handler
  * @param fill_buffer_func The function to fill the buffer with read data
  * @param p_remote_port (Optional) The remote port from where the data was
  * received
  */
  basic_pending_dgr_read_operation(
    basic_pending_io_operation::func_type func,
    fill_dgr_buffer_func_type fill_dgr_buffer_func,
    fiber_id::remote_port_type* p_remote_port = nullptr)
    : basic_pending_io_operation(func),
      fill_dgr_buffer_func_(fill_dgr_buffer_func),
      p_remote_port_(p_remote_port) {
  }

public:
  /// Function called to fill the buffer with read data
  /**
  * @param buf The buffer where to store the read data
  */
  size_t fill_buffers(const std::vector<uint8_t>& buf) {
    if (fill_dgr_buffer_func_) {
      return fill_dgr_buffer_func_(this, buf);
    } else {
      return 0;
    }
  }

  /// Set the remote port
  /**
  * @param remote_port The remote port value to set
  */
  bool set_remote_port(const fiber_id::remote_port_type& remote_port) {
    if (p_remote_port_) {
      *p_remote_port_ = remote_port;
      return true;
    } else {
      return false;
    }
  }

private:
  fill_dgr_buffer_func_type fill_dgr_buffer_func_;
  fiber_id::remote_port_type* p_remote_port_;
};

/// Base class for pending ssl read operations
class basic_pending_ssl_operation : public basic_pending_io_operation
{
private:
  typedef size_t(*fill_buffer_func_type)(basic_pending_ssl_operation*,
                                          boost::asio::streambuf&);

protected:
  /// Constructor
  /**
  * @param func The completion handler
  * @param fill_buffer_func The function to fill the buffer with read data
  */
  basic_pending_ssl_operation(basic_pending_io_operation::func_type func,
                              fill_buffer_func_type fill_buffer_func)
                              : basic_pending_io_operation(func),
                                fill_buffer_func_(fill_buffer_func)
  {
  }

public:
  /// Function called to fill the buffer with read data
  /**
  * @param buf The buffer where to store the read data
  */
  size_t fill_buffers(boost::asio::streambuf& buf)
  {
    if (fill_buffer_func_)
    {
      return fill_buffer_func_(this, buf);
    }
    else
    {
      return 0;
    }
  }

private:
  fill_buffer_func_type fill_buffer_func_;
};

} // namespace detail
} // namespace fiber
} // namespace asio
} // namespace boost

#endif  // SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_IO_OPERATION_HPP_
