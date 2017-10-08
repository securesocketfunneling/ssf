#include <cstdint>
#include <thread>

#include "core/async_engine.h"

#include "ssf/log/log.h"

namespace ssf {

AsyncEngine::AsyncEngine() : io_service_(), p_worker_(nullptr), threads_() {}

AsyncEngine::~AsyncEngine() { Stop(); }

boost::asio::io_service& AsyncEngine::get_io_service() { return io_service_; }

void AsyncEngine::Start() {
  if (IsStarted()) {
    return;
  }

  SSF_LOG(kLogDebug) << "async engine: starting";
  p_worker_.reset(new boost::asio::io_service::work(io_service_));
  for (uint8_t i = 0; i < std::thread::hardware_concurrency(); ++i) {
    threads_.emplace_back([this]() {
      boost::system::error_code ec;
      io_service_.run(ec);
      if (ec) {
        SSF_LOG(kLogError) << "async engine: run io_service failed: "
                           << ec.message();
      }
    });
  }
}

void AsyncEngine::Stop() {
  if (!IsStarted()) {
    return;
  }

  SSF_LOG(kLogDebug) << "async engine: stopping";
  p_worker_.reset(nullptr);
  for (auto& thread : threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }
  io_service_.stop();
  io_service_.reset();
}

bool AsyncEngine::IsStarted() const { return p_worker_.get() != nullptr; }

}  // ssf
