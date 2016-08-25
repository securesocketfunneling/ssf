#ifndef SSF_IO_OP_H_
#define SSF_IO_OP_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif  // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/handler_tracking.hpp>
#include <boost/system/error_code.hpp>

namespace ssf {
namespace io {

/// Base class for pending io operations without size information
class basic_pending_io_operation BOOST_ASIO_INHERIT_TRACKED_HANDLER {
 public:
  /// Function called on completion
  /**
  * @param ec The error code resulting of the completed operation
  */
  void complete(const boost::system::error_code& ec) {
    auto destroy = false;
    func_(this, destroy, ec);
  }

  void destroy() {
    auto destroy = true;
    func_(this, destroy, boost::system::error_code());
  }

 protected:
  typedef void (*func_type)(basic_pending_io_operation*, bool,
                            const boost::system::error_code& ec);

  basic_pending_io_operation(func_type func) : next_(0), func_(func) {}

  ~basic_pending_io_operation() {}

  friend class boost::asio::detail::op_queue_access;
  basic_pending_io_operation* next_;
  func_type func_;
};

/// Base class for pending io operations with size information
class basic_pending_sized_io_operation BOOST_ASIO_INHERIT_TRACKED_HANDLER {
public:
  /// Function called on completion
  /**
  * @param ec The error code resulting of the completed operation
  */
  void complete(const boost::system::error_code& ec, std::size_t length) {
    auto destroy = false;
    func_(this, destroy, ec, length);
  }

  void destroy() {
    auto destroy = true;
    func_(this, destroy, boost::system::error_code(), 0);
  }

protected:
  typedef void(*func_type)(basic_pending_sized_io_operation*, bool,
    const boost::system::error_code& ec, std::size_t length);

  basic_pending_sized_io_operation(func_type func) : next_(0), func_(func) {}

  ~basic_pending_sized_io_operation() {}

  friend class boost::asio::detail::op_queue_access;
  basic_pending_sized_io_operation* next_;
  func_type func_;
};

}  // io
}  // ssf

#endif  // SSF_IO_OP_H_
