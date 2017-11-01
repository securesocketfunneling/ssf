#include <cstdint>
#include <thread>

#include "core/async_engine.h"

#include "ssf/log/log.h"

namespace ssf {

AsyncEngine::AsyncEngine()
    : io_service_(), p_worker_(nullptr), threads_(), is_started_(false) {}

AsyncEngine::~AsyncEngine() { Stop(); }

boost::asio::io_service& AsyncEngine::get_io_service() { return io_service_; }

void AsyncEngine::Start() {
  if (is_started_) {
    return;
  }

  SSF_LOG("async_engine", debug, "starting");
  is_started_ = true;
  p_worker_.reset(new boost::asio::io_service::work(io_service_));
  for (uint8_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
    threads_.emplace_back([this]() {
      boost::system::error_code ec;
      io_service_.run(ec);
      if (ec) {
        SSF_LOG("async_engine", error, "run io_service failed: {}",
                ec.message());
      }
    });
  }
}

void AsyncEngine::Stop() {
  if (!is_started_) {
    return;
  }

  SSF_LOG("async_engine", debug, "stop");
  p_worker_.reset(nullptr);
  for (auto& thread : threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }
  io_service_.stop();
  io_service_.reset();
  is_started_ = false;
}

bool AsyncEngine::IsStarted() const { return is_started_; }

}  // ssf
