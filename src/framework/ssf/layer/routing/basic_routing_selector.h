#ifndef SSF_LAYER_ROUTING_BASIC_ROUTING_SELECTOR_H_
#define SSF_LAYER_ROUTING_BASIC_ROUTING_SELECTOR_H_

#include <utility>

#include <boost/system/error_code.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/async_result.hpp>

#include "ssf/error/error.h"

namespace ssf {
namespace layer {
namespace routing {

template <class Identifier, class Element, class TRoutingTable>
class RoutingSelector {
 public:
  typedef TRoutingTable RoutingTable;

 public:
  RoutingSelector() : p_routing_table_(nullptr) {}

  RoutingSelector(RoutingSelector&& other)
      : p_routing_table_(std::move(other.p_routing_selector)) {}

  void set_routing_table(RoutingTable* p_routing_table) {
    p_routing_table_ = p_routing_table;
  }

  /// Update p_id by resolving the destination id of the element
  bool operator()(Identifier* p_id, Element* p_element) const {
    boost::system::error_code ec;

    *p_id = p_routing_table_->Resolve(
        p_element->header().id().GetSecondHalfId(), ec);

    return !ec;
  }

 private:
  RoutingTable* p_routing_table_;
};

}  // routing
}  // layer
}  // ssf

#endif  // SSF_LAYER_ROUTING_BASIC_ROUTING_SELECTOR_H_
