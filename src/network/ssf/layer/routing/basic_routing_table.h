#ifndef SSF_LAYER_ROUTING_BASIC_ROUTING_TABLE_H_
#define SSF_LAYER_ROUTING_BASIC_ROUTING_TABLE_H_

#include <cstdint>

#include <map>
#include <mutex>

#include <boost/system/error_code.hpp>

#include "ssf/error/error.h"

#include "ssf/log/log.h"

namespace ssf {
namespace layer {
namespace routing {

template <class NetworkProtocol>
class basic_RoutingTable {
 private:
  typedef typename NetworkProtocol::endpoint_context_type network_address_type;
  typedef network_address_type prefix_type;

 public:
  boost::system::error_code AddRoute(
      prefix_type prefix, network_address_type network_endpoint_context,
      boost::system::error_code& ec) {
    std::unique_lock<std::recursive_mutex> lock(mutex_);

    SSF_LOG(kLogTrace) << " * Routing table : add route from " << prefix
                       << " to " << network_endpoint_context;

    auto inserted = table_.insert(
        std::make_pair(std::move(prefix), std::move(network_endpoint_context)));

    if (!inserted.second) {
      ec.assign(ssf::error::address_in_use, ssf::error::get_ssf_category());
    }

    ec.assign(ssf::error::success, ssf::error::get_ssf_category());
    return ec;
  }

  boost::system::error_code RemoveRoute(const prefix_type& prefix,
                                        boost::system::error_code& ec) {
    std::unique_lock<std::recursive_mutex> lock(mutex_);

    auto erased = table_.erase(prefix);

    if (!erased) {
      ec.assign(ssf::error::not_connected, ssf::error::get_ssf_category());
    } else {
      ec.assign(ssf::error::success, ssf::error::get_ssf_category());
    }

    return ec;
  }

  /// Resolve a network id and return the associated endpoint
  network_address_type Resolve(const prefix_type& prefix,
                               boost::system::error_code& ec) const {
    std::unique_lock<std::recursive_mutex> lock(mutex_);

    auto network_endpoint_context_it = table_.find(prefix);

    if (network_endpoint_context_it == std::end(table_)) {
      ec.assign(ssf::error::not_connected, ssf::error::get_ssf_category());
      return network_address_type();
    }

    ec.assign(ssf::error::success, ssf::error::get_ssf_category());
    return network_endpoint_context_it->second;
  }

  /// Clear the routing table
  boost::system::error_code Flush(boost::system::error_code& ec) {
    std::unique_lock<std::recursive_mutex> lock(mutex_);

    table_.clear();

    ec.assign(ssf::error::success, ssf::error::get_ssf_category());
    return ec;
  }

 private:
  mutable std::recursive_mutex mutex_;
  std::map<prefix_type, network_address_type> table_;
};

}  // routing
}  // layer
}  // ssf

#endif  // SSF_LAYER_ROUTING_BASIC_ROUTING_TABLE_H_