#ifndef SSF_CORE_ASYNC_RUNNER_H_
#define SSF_CORE_ASYNC_RUNNER_H_

#include <boost/asio/io_service.hpp>
#include <boost/thread.hpp>

namespace ssf {

class AsyncRunner {
 public:
  AsyncRunner();
  virtual ~AsyncRunner();

  AsyncRunner(const AsyncRunner&) = delete;
  AsyncRunner& operator=(const AsyncRunner&) = delete;

  void StartAsyncEngine();
  void StopAsyncEngine();

 protected:
  boost::asio::io_service io_service_;

 private:
  boost::thread_group threads_;
};

}  // ssf

#endif  // SSF_CORE_ASYNC_RUNNER_H_
