#ifndef SSF_LAYER_CONGESTION_DROP_TAIL_POLICY_H_
#define SSF_LAYER_CONGESTION_DROP_TAIL_POLICY_H_

namespace ssf {
namespace layer {
namespace congestion {

template<uint32_t MaxSize>
class DropTailPolicy {
 public:
  template<class Queue, class Packet>
  bool IsAddable(const Queue& queue, const Packet& packet) {
    return queue.size() < MaxSize;
  }

  template <class Queue>
  bool IsAddable(const Queue& queue) {
    return queue.size() < MaxSize;
  }
};

}  // congestion
}  // layer
}  // ssf

#endif  // SSF_LAYER_CONGESTION_DROP_TAIL_POLICY_H_
