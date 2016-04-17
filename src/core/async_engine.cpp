#include "core/async_engine.h"

#include <stdint.h>

#include <boost/log/trivial.hpp>

namespace ssf {

AsyncEngine::AsyncEngine() : io_service_(), p_worker_(nullptr), threads_() {}

AsyncEngine::~AsyncEngine() { Stop(); }

boost::asio::io_service& AsyncEngine::get_io_service() { return io_service_; }

void AsyncEngine::Start() {
  if (IsStarted()) {
    return;
  }

  p_worker_.reset(new boost::asio::io_service::work(io_service_));
  for (uint8_t i = 0; i < boost::thread::hardware_concurrency(); ++i) {
    threads_.create_thread([this]() {
      boost::system::error_code ec;
      io_service_.run(ec);
      if (ec) {
        BOOST_LOG_TRIVIAL(error)
            << "async engine : when running io_service : " << ec.message();
      }
    });
  }
}

void AsyncEngine::Stop() {
  p_worker_.reset(nullptr);
  threads_.join_all();
  io_service_.stop();
  io_service_.reset();
}

bool AsyncEngine::IsStarted() const { return p_worker_.get() != nullptr; }

}  // ssf
