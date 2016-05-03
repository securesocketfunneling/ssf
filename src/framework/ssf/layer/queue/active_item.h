#ifndef SSF_LAYER_QUEUE_ACTIVE_ITEM_H_
#define SSF_LAYER_QUEUE_ACTIVE_ITEM_H_

#include <memory>

namespace ssf {
namespace layer {
namespace queue {

template <class Item>
class ActiveItem {
 public:
  ActiveItem(Item item) : active_(true), item_(std::move(item)) {}

  bool IsActive() const { return active_; }

  void Disactivate() { active_ = false; }

  const Item& item() const { return item_; }
  Item& item() { return item_; }

 private:
  bool active_;
  Item item_;
};

template <class Item>
using ActiveItemPtr = std::shared_ptr<ActiveItem<Item>>;

template <class Item>
ActiveItem<typename std::remove_reference<Item>::type> make_active(
    Item&& item) {
  return ActiveItem<typename std::remove_reference<Item>::type>(
      std::forward<Item>(item));
}

template <class Item>
ActiveItemPtr<typename std::remove_reference<Item>::type> make_shared_active(
    Item&& item) {
  return std::make_shared<
      ActiveItem<typename std::remove_reference<Item>::type>>(
      std::forward<Item(item)>);
}

}  // queue
}  // layer
}  // ssf

#endif  // SSF_LAYER_QUEUE_ACTIVE_ITEM_H_
