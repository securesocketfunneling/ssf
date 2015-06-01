#ifndef SSF_COMMON_NETWORK_BASE_SESSION_H_
#define SSF_COMMON_NETWORK_BASE_SESSION_H_

#include <memory>

#include <boost/system/error_code.hpp>

namespace ssf {
/// Base class for ActionableItem types
class BaseSession : public std::enable_shared_from_this<BaseSession> {
 public:
  BaseSession() {}
  virtual void start(boost::system::error_code&) = 0;
  virtual void stop(boost::system::error_code&) = 0;
  virtual ~BaseSession() {}

 private:
  // Make non-copyable
  BaseSession(const BaseSession&);
  BaseSession& operator=(const BaseSession&);
};

typedef std::shared_ptr<BaseSession> BaseSessionPtr;
}  // ssf

#endif  // SSF_COMMON_NETWORK_BASE_SESSION_H_
