#ifndef SSF_COMMON_IO_GET_OP_H_
#define SSF_COMMON_IO_GET_OP_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif  // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/addressof.hpp>
#include <boost/asio/detail/bind_handler.hpp>
#include <boost/asio/detail/fenced_block.hpp>
#include <boost/asio/detail/handler_alloc_helpers.hpp>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/error.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace io {

template <class T>
class basic_pending_get_operation BOOST_ASIO_INHERIT_TRACKED_HANDLER {
 public:
  void complete(const boost::system::error_code& ec, T element) {
    auto destroy = false;
    func_(this, destroy, ec, std::move(element));
  }

  void destroy() {
    auto destroy = true;
    func_(this, destroy, boost::system::error_code(), T());
  }

 protected:
  typedef void (*func_type)(basic_pending_get_operation*, bool,
                            const boost::system::error_code&, T);

  basic_pending_get_operation(func_type func) : next_(nullptr), func_(func) {}

  ~basic_pending_get_operation() {}

  friend class boost::asio::detail::op_queue_access;
  basic_pending_get_operation* next_;
  func_type func_;
};

template <class Handler, class T>
class pending_get_operation : public basic_pending_get_operation<T> {
 public:
  BOOST_ASIO_DEFINE_HANDLER_PTR(pending_get_operation);

  pending_get_operation(Handler handler)
      : basic_pending_get_operation<T>(&pending_get_operation::do_complete),
        handler_(std::move(handler)) {}

  static void do_complete(basic_pending_get_operation<T>* base, bool destroy,
                          const boost::system::error_code& result_ec,
                          T element) {
    boost::system::error_code ec(result_ec);

    pending_get_operation* o(static_cast<pending_get_operation*>(base));

    ptr p = {boost::asio::detail::addressof(o->handler_), o, o};

    BOOST_ASIO_HANDLER_COMPLETION((o));

    boost::asio::detail::binder2<Handler, boost::system::error_code, T> handler(
        o->handler_, ec, std::move(element));
    p.h = boost::asio::detail::addressof(handler.handler_);
    p.reset();

    if (!destroy) {
      boost::asio::detail::fenced_block b(
          boost::asio::detail::fenced_block::half);
      BOOST_ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_, handler.arg2_));
      boost_asio_handler_invoke_helpers::invoke(handler, handler.handler_);
      BOOST_ASIO_HANDLER_INVOCATION_END;
    }
  }

 private:
  Handler handler_;
};

}  // io

#include <boost/asio/detail/pop_options.hpp>

#endif  // SSF_COMMON_IO_GET_OP_H_
