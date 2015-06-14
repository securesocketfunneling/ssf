#ifndef SSF_CORE_VIRTUAL_NETWORK_BASIC_RESOLVER_H_
#define SSF_CORE_VIRTUAL_NETWORK_BASIC_RESOLVER_H_

#include <vector>
#include <list>

#include <boost/asio/io_service.hpp>

#include <boost/system/error_code.hpp>

namespace virtual_network {

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
  typedef typename Protocol::raw_endpoint_parameters parameters_type;
  typedef typename Protocol::endpoint endpoint_type;
  typedef EndpointIterator iterator;

 public:
  basic_VirtualLink_resolver(boost::asio::io_service& io_service)
      : io_service_(io_service) {}

  template <class Container>
  iterator resolve(
      const Container& parameters_list,
      boost::system::error_code& ec = boost::system::error_code()) {
    if (parameters_list.size() < Protocol::endpoint_stack_size) {
      ec.assign(ssf::error::invalid_argument, ssf::error::get_ssf_category());
      return iterator();
    }

    std::vector<endpoint_type> result;
    result.emplace_back(Protocol::template make_endpoint<Container>(
        io_service_, std::begin(parameters_list), 0, ec));

    if (ec) {
      return iterator();
    }

    return iterator(result);
  }

 private:
  boost::asio::io_service& io_service_;
};

}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_BASIC_RESOLVER_H_
