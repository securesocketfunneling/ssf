#ifndef SSF_NETWORK_MANAGER_H
#define SSF_NETWORK_MANAGER_H

#include <cstdint>

#include <map>
#include <set>
#include <limits>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/system/error_code.hpp>

#include "ssf/error/error.h"

namespace ssf {

/// Manage actionable items
/** Items must support start() and stop() interfaces and
* implement operator==
*/
template <typename ActionableItem>
class ItemManager : private boost::noncopyable {

 public:
  /// Type for the unique instance id given to each item managed
  typedef uint32_t instance_id_type;

 public:
  ItemManager() : id_map_mutex_(), id_map_() {}

  ~ItemManager() { do_stop_all(); }

  /// Activate an Item and return a unique ID
  instance_id_type start(ActionableItem item, boost::system::error_code& ec) {
    return do_start(item, ec);
  }

  /// Stop an item
  void stop(ActionableItem item, boost::system::error_code& ec) {
    do_stop(find_id_from_item(item), ec);
  }

  /// Stop the item given its unique ID
  void stop_with_id(instance_id_type id, boost::system::error_code& ec) {
    do_stop(id, ec);
  }

  /// Stop all items
  void stop_all() { do_stop_all(); }

 private:
  /// Find the unique ID of the given item
  instance_id_type find_id_from_item(ActionableItem item) {
    boost::recursive_mutex::scoped_lock lock(id_map_mutex_);

    for (const auto& item_pair : id_map_) {
      if (item_pair.second == item) {
        return item_pair.first;
      }
    }

    return 0;
  }

  /// Activate the item and return a unique ID
  instance_id_type do_start(ActionableItem item,
                            boost::system::error_code& ec) {
    boost::recursive_mutex::scoped_lock lock(id_map_mutex_);

    /// Get an available ID
    instance_id_type new_id = get_available_id();

    /// If new_id == 0, no ID was available
    if (!new_id) {
      ec.assign(ssf::error::device_or_resource_busy,
                ssf::error::get_ssf_category());
      return 0;
    } else {
      item->start(ec);
      if (!ec) {
        id_map_.insert(std::make_pair(new_id, std::move(item)));
        return new_id;
      } else {
        return 0;
      }
    }
  }

  /// Stop the item associated to the given unique ID
  void do_stop(instance_id_type id, boost::system::error_code& ec) {
    boost::recursive_mutex::scoped_lock lock(id_map_mutex_);

    auto it = id_map_.find(id);

    if (it != std::end(id_map_)) {
      it->second->stop(ec);
      if (!ec) {
        id_map_.erase(id);
      }
    } else {
      ec.assign(ssf::error::invalid_argument, ssf::error::get_ssf_category());
    }
  }

  /// Stop all items
  void do_stop_all() {
    boost::recursive_mutex::scoped_lock lock(id_map_mutex_);

    boost::system::error_code ec;
    for (auto& item : id_map_) {
      item.second->stop(ec);
    }
    id_map_.clear();
  }

  /// Return the next available ID and 0 if no ID is available
  instance_id_type get_available_id() {
    boost::recursive_mutex::scoped_lock lock(id_map_mutex_);

    for (instance_id_type i = 1; i < std::numeric_limits<uint32_t>::max();
         ++i) {
      if (!id_map_.count(i)) {
        return i;
      }
    }
    return 0;
  }

 private:
  boost::recursive_mutex id_map_mutex_;
  std::map<instance_id_type, ActionableItem> id_map_;
};

}  // ssf

#endif  // SSF_NETWORK_MANAGER_H
