#include "core/async_runner.h"

#include <stdint.h>

#include <boost/log/trivial.hpp>

namespace ssf {

AsyncRunner::AsyncRunner() : io_service_(), threads_() {}

AsyncRunner::~AsyncRunner() {}

void AsyncRunner::StartAsyncEngine() {
  for (uint8_t i = 0; i < boost::thread::hardware_concurrency(); ++i) {
    threads_.create_thread([this]() {
      boost::system::error_code ec;
      io_service_.run(ec);
      if (ec) {
        BOOST_LOG_TRIVIAL(error)
            << "error running io_service : " << ec.message();
      }
    });
  }
}

void AsyncRunner::StopAsyncEngine() {
  threads_.join_all();
  io_service_.stop();
  io_service_.reset();
}

}  // ssf
