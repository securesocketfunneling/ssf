#ifndef SSF_CORE_VIRTUAL_NETWORK_IO_HANDLER_H_
#define SSF_CORE_VIRTUAL_NETWORK_IO_HANDLER_H_

#include <memory>

#include <boost/system/error_code.hpp>

namespace virtual_network {

class BaseIOHandler {
 public:
  virtual ~BaseIOHandler() {}
  virtual void operator()(const boost::system::error_code&, std::size_t) = 0;
  virtual void operator()(const boost::system::error_code&, std::size_t) const = 0;
};

template <class SubHandler>
class IOHandler : public BaseIOHandler {
 public:
  IOHandler() {}
  IOHandler(SubHandler handler) : handler_(std::move(handler)) {}

  void operator()(const boost::system::error_code& ec, std::size_t length) {
    handler_(ec, length);
  }

  void operator()(const boost::system::error_code& ec, std::size_t length) const {
    handler_(ec, length);
  }

 private:
  SubHandler handler_;
};

typedef std::shared_ptr<BaseIOHandler> BaseIOHandlerPtr;

class WrappedIOHandler {
public:
 WrappedIOHandler() : p_handler_(nullptr) {}

 template <class Handler>
 WrappedIOHandler(Handler handler)
     : p_handler_(std::make_shared<IOHandler<Handler>>(std::move(handler))) {}

 WrappedIOHandler(const WrappedIOHandler& other)
     : p_handler_(other.p_handler_) {}

 WrappedIOHandler(WrappedIOHandler&& other)
     : p_handler_(std::move(other.p_handler_)) {}

 ~WrappedIOHandler() {}

 void operator()(const boost::system::error_code& ec, std::size_t length) {
   if (p_handler_) {
     (*p_handler_)(ec, length);
   }
 }

 void operator()(const boost::system::error_code& ec,
                 std::size_t length) const {
   if (p_handler_) {
     (*p_handler_)(ec, length);
   }
 }

private:
 BaseIOHandlerPtr p_handler_;
};

}  // virtual_network
#endif  // SSF_CORE_VIRTUAL_NETWORK_IO_HANDLER_H_
