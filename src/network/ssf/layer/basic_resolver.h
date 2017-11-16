#ifndef SSF_LAYER_BASIC_RESOLVER_H_
#define SSF_LAYER_BASIC_RESOLVER_H_

#include <vector>

#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

#include "ssf/error/error.h"

#include "ssf/layer/parameters.h"

namespace ssf {
namespace layer {

template <class Protocol>
class basic_VirtualLink_resolver {
 private:
  class EndpointIterator {
   public:
    EndpointIterator() : endpoints_(1), index_(0) {}
    EndpointIterator(std::vector<typename Protocol::endpoint> endpoints)
        : endpoints_(endpoints), index_(0) {}

    typename Protocol::endpoint& operator*() { return endpoints_[index_]; }
    typename Protocol::endpoint* operator->() { return &endpoints_[index_]; }

    typename Protocol::endpoint& operator++() {
      ++index_;
      return endpoints_[index_];
    }

    typename Protocol::endpoint operator++(int) {
      ++index_;
      return endpoints_[index_ - 1];
    }

    typename Protocol::endpoint& operator--() {
      --index_;
      return endpoints_[index_];
    }

    typename Protocol::endpoint operator--(int) {
      --index_;
      return endpoints_[index_ + 1];
    }

   private:
    std::vector<typename Protocol::endpoint> endpoints_;
    std::size_t index_;
  };

 public:
  typedef typename Protocol::endpoint endpoint_type;
  typedef EndpointIterator iterator;
  using query = ParameterStack;

 public:
  basic_VirtualLink_resolver(boost::asio::io_service& io_service)
      : io_service_(io_service) {}

  iterator resolve(const query& parameters_list,
                   boost::system::error_code& ec) {
    if (parameters_list.size() < Protocol::endpoint_stack_size) {
      ec.assign(ssf::error::invalid_argument, ssf::error::get_ssf_category());
      return iterator();
    }

    std::vector<endpoint_type> result;
    result.emplace_back(Protocol::make_endpoint(
        io_service_, std::begin(parameters_list), 0, ec));

    if (ec) {
      return iterator();
    }

    return iterator(result);
  }

 private:
  boost::asio::io_service& io_service_;
};

}  // layer
}  // ssf

#endif  // SSF_LAYER_BASIC_RESOLVER_H_
