#ifndef SSF_LAYER_QUEUE_ASYNC_QUEUE_H_
#define SSF_LAYER_QUEUE_ASYNC_QUEUE_H_

#include <cstdint>

#include <queue>

#include <boost/asio/basic_io_object.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/async_result.hpp>

#include <boost/integer_traits.hpp>

#include "ssf/layer/queue/async_queue_service.h"

namespace ssf {
namespace layer {
namespace queue {

template <class Ttype, class Container = std::queue<Ttype>,
          uint32_t QueueMaxSize = boost::integer_traits<uint32_t>::const_max,
          uint32_t OPQueueMaxSize = boost::integer_traits<uint32_t>::const_max,
          class Service = basic_async_queue_service<Ttype, Container, QueueMaxSize,
                                                    OPQueueMaxSize>>
class basic_async_queue : public boost::asio::basic_io_object<Service> {
 private:
  typedef Ttype T;

 public:
  typedef typename Service::value_type value_type;
  typedef typename Service::container_type container_type;
  enum {
    kQueueMaxSize = Service::kQueueMaxSize,
    kOPQueueMaxSize = Service::kOPQueueMaxSize
  };

 public:
  basic_async_queue(boost::asio::io_service& io_service)
    : boost::asio::basic_io_object<Service>(io_service) {}

  basic_async_queue(const basic_async_queue&) = delete;
  basic_async_queue& operator=(const basic_async_queue&) = delete;

  basic_async_queue(basic_async_queue&& other)
      : boost::asio::basic_io_object<Service>(std::move(other)) {}

  basic_async_queue& operator=(basic_async_queue&& other){
    boost::asio::basic_io_object<Service>::operator=(std::move(other));
    return *this;
  }

  ~basic_async_queue() {}

  boost::system::error_code push(T element, boost::system::error_code& ec) {
    return this->get_service().push(this->implementation,
                                           std::move(element), ec);
  }

  template <class Handler>
  BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(boost::system::error_code))
      async_push(T element, Handler&& handler) {
    return this->get_service().async_push(this->implementation,
                                          std::move(element),
                                          std::forward<Handler>(handler));
  }

  T get(boost::system::error_code& ec) {
    return this->get_service().get(this->implementation, ec);
  }

  template <class Handler>
  BOOST_ASIO_INITFN_RESULT_TYPE(Handler, void(boost::system::error_code,
                                              T)) async_get(Handler&& handler) {
    return this->get_service().async_get(this->implementation,
                                         std::forward<Handler>(handler));
  }

  bool empty() const { return this->get_service().empty(this->implementation); }

  std::size_t size() const {
    return this->get_service().size(this->implementation);
  }

  void clear() { return this->get_service().clear(this->implementation); }

  boost::system::error_code close(boost::system::error_code& ec) {
    return this->get_service().close(this->implementation, ec);
  }
};

}  // queue
}  // layer
}  // ssf

#endif  // SSF_LAYER_QUEUE_ASYNC_QUEUE_H_
