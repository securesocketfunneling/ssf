#ifndef SSF_LAYER_QUEUE_TAGGED_ITEM_H_
#define SSF_LAYER_QUEUE_TAGGED_ITEM_H_

#include <memory>

namespace ssf {
namespace layer {
namespace queue {

template <class Tag, class Item>
struct TaggedItem {
  TaggedItem(Tag t, Item i) : tag(std::move(t)), item(std::move(i)) {}

  Tag tag;
  Item item;
};

template <class Tag, class Item>
using TaggedItemPtr = std::shared_ptr<TaggedItem<Tag, Item>>;

template <class Tag, class Item>
TaggedItem<typename std::remove_reference<Tag>::type,
           typename std::remove_reference<Item>::type>
make_tagged(Tag&& tag, Item&& item) {
  return TaggedItem<typename std::remove_reference<Tag>::type,
                    typename std::remove_reference<Item>::type>{
      std::forward<Tag>(tag), std::forward<Item>(item)};
}

template <class Tag, class Item>
TaggedItemPtr<typename std::remove_reference<Tag>::type,
              typename std::remove_reference<Item>::type>
make_shared_tagged(Tag&& tag, Item&& item) {
  return std::make_shared<
      TaggedItem<typename std::remove_reference<Tag>::type,
                 typename std::remove_reference<Item>::type>>(
      std::forward<Tag>(tag), std::forward<Item>(item));
}

}  // queue
}  // layer
}  // ssf

#endif  // SSF_LAYER_QUEUE_TAGGED_ITEM_H_
