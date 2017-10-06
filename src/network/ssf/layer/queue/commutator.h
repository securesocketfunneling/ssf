#ifndef SSF_LAYER_QUEUE_COMMUTATOR_H_
#define SSF_LAYER_QUEUE_COMMUTATOR_H_

#include <functional>
#include <map>
#include <memory>
#include <mutex>

#include <boost/system/error_code.hpp>

#include "ssf/error/error.h"

#include "ssf/layer/queue/active_item.h"
#include "ssf/layer/queue/tagged_item.h"

#include "ssf/log/log.h"

namespace ssf {
namespace layer {
namespace queue {

template <class TIdentifier, class TElement, class TSelector>
class Commutator : public std::enable_shared_from_this<
                       Commutator<TIdentifier, TElement, TSelector>> {
 public:
  typedef TIdentifier Identifier;
  typedef TElement Element;
  typedef TSelector Selector;

  typedef TaggedItem<Identifier, Element> TaggedElement;

  typedef std::function<void(const boost::system::error_code&, Element)>
      InputHandler;
  typedef std::function<void(InputHandler)> InputCallback;
  typedef TaggedItemPtr<Identifier, ActiveItem<InputCallback>>
      TaggedActiveInputCallbackPtr;

  typedef std::function<void(const boost::system::error_code&)> OutputHandler;
  typedef std::function<void(Element, OutputHandler)> OutputCallback;

  typedef std::map<Identifier, TaggedActiveInputCallbackPtr> InputCallbackMap;
  typedef std::map<Identifier, OutputCallback> OutputCallbackMap;

 public:
  template <class... Args>
  static std::shared_ptr<Commutator> Create(Args&&... args) {
    return std::shared_ptr<Commutator>(
        new Commutator(std::forward<Args>(args)...));
  }

  ~Commutator() {}

  bool RegisterInput(Identifier id, InputCallback input_callback) {
    std::unique_lock<std::recursive_mutex> lock(input_callbacks_mutex_);

    auto active_item = make_active(std::move(input_callback));
    auto p_tagged_active_item = make_shared_tagged(id, std::move(active_item));

    auto inserted = input_callbacks_.insert(
        std::make_pair(std::move(id), p_tagged_active_item));

    StartInputLoop(std::move(p_tagged_active_item));

    return inserted.second;
  }

  bool UnregisterInput(const Identifier& id) {
    std::unique_lock<std::recursive_mutex> lock(input_callbacks_mutex_);

    auto p_tagged_active_item_it = input_callbacks_.find(id);

    if (p_tagged_active_item_it == std::end(input_callbacks_)) {
      return false;
    }

    p_tagged_active_item_it->second->item.Disactivate();
    input_callbacks_.erase(p_tagged_active_item_it);

    return true;
  }

  bool RegisterOutput(Identifier id, OutputCallback output_callback) {
    std::unique_lock<std::recursive_mutex> lock(output_callbacks_mutex_);

    auto inserted = output_callbacks_.insert(
        std::make_pair(std::move(id), std::move(output_callback)));

    return inserted.second;
  }

  bool UnregisterOutput(const Identifier& id) {
    std::unique_lock<std::recursive_mutex> lock(output_callbacks_mutex_);

    auto p_output_it = output_callbacks_.find(id);

    if (p_output_it == std::end(output_callbacks_)) {
      return false;
    }

    output_callbacks_.erase(p_output_it);

    return true;
  }

  boost::system::error_code close(boost::system::error_code& ec) {
    {
      std::unique_lock<std::recursive_mutex> lock(input_callbacks_mutex_);

      for (auto& input_callback : input_callbacks_) {
        input_callback.second->item.Disactivate();
      }

      input_callbacks_.clear();
    }

    {
      std::unique_lock<std::recursive_mutex> lock(output_callbacks_mutex_);
      output_callbacks_.clear();
    }

    return ec;
  }

 private:
  Commutator(boost::asio::io_service& io_service, const Selector& selector)
      : io_service_(io_service),
        selector_(selector),
        input_callbacks_mutex_(),
        input_callbacks_(),
        output_callbacks_mutex_(),
        output_callbacks_() {}

 private:
  void StartInputLoop(TaggedActiveInputCallbackPtr p_input_callback) {
    p_input_callback->item.item()(std::bind(
        &Commutator::InputReceived, this->shared_from_this(), p_input_callback,
        std::placeholders::_1, std::placeholders::_2));
  }

  void InputReceived(TaggedActiveInputCallbackPtr p_input_callback,
                     const boost::system::error_code& ec, Element element) {
    if (ec) {
      p_input_callback->item.Disactivate();
      SSF_LOG(kLogTrace) << " * Deactivate input received callback";
      // TODO : erase from map?
      //        continue routing when socket is disconnected ?

      return;
    }

    AsyncCommute(p_input_callback->tag, std::move(element),
                 std::bind(&Commutator::OutputSent, this->shared_from_this(),
                           std::placeholders::_1));

    StartInputLoop(std::move(p_input_callback));
  }

  void AsyncCommute(Identifier id, Element element, OutputHandler handler) {
    auto tagged_element = make_tagged(std::move(id), std::move(element));

    io_service_.post(std::bind(&Commutator::Select, this->shared_from_this(),
                               std::move(tagged_element), std::move(handler)));
  }

  void Select(TaggedElement tagged_element, OutputHandler handler) {
    auto selected = selector_(&tagged_element.tag, &tagged_element.item);

    if (!selected) {
      handler(boost::system::error_code(ssf::error::not_connected,
                                        ssf::error::get_ssf_category()));
      return;
    }
    DoOutput(std::move(tagged_element), std::move(handler));
  }

  void DoOutput(TaggedElement tagged_element, OutputHandler handler) {
    std::unique_lock<std::recursive_mutex> lock(output_callbacks_mutex_);

    auto output_it = output_callbacks_.find(tagged_element.tag);

    if (output_it == std::end(output_callbacks_)) {
      handler(boost::system::error_code(ssf::error::not_connected,
                                        ssf::error::get_ssf_category()));
      return;
    }

    output_it->second(std::move(tagged_element.item), std::move(handler));
  }

  void OutputSent(const boost::system::error_code& ec) {}

 private:
  boost::asio::io_service& io_service_;

  const Selector& selector_;

  std::recursive_mutex input_callbacks_mutex_;
  InputCallbackMap input_callbacks_;

  std::recursive_mutex output_callbacks_mutex_;
  OutputCallbackMap output_callbacks_;
};

template <class Identifier, class Element, class Selector>
using CommutatorPtr =
    std::shared_ptr<Commutator<Identifier, Element, Selector>>;

}  // queue
}  // layer
}  // ssf

#endif  // SSF_LAYER_QUEUE_COMMUTATOR_H_
