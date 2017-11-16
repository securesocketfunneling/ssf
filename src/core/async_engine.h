#ifndef SSF_CORE_ASYNC_ENGINE_H_
#define SSF_CORE_ASYNC_ENGINE_H_

#include <memory>
#include <thread>
#include <vector>

#include <boost/asio/io_service.hpp>

namespace ssf {

class AsyncEngine {
 private:
  using WorkerPtr = std::unique_ptr<boost::asio::io_service::work>;

 public:
  AsyncEngine();
  ~AsyncEngine();

  AsyncEngine(const AsyncEngine&) = delete;
  AsyncEngine& operator=(const AsyncEngine&) = delete;

  boost::asio::io_service& get_io_service();

  void Start();
  void Stop();

  bool IsStarted() const;

 private:
  boost::asio::io_service io_service_;
  WorkerPtr p_worker_;
  std::vector<std::thread> threads_;
  bool is_started_;
};

}  // ssf

#endif  // SSF_CORE_ASYNC_RUNNER_H_
