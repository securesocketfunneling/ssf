#ifndef SSF_CORE_VIRTUAL_NETWORK_BASIC_ENDPOINT_H_
#define SSF_CORE_VIRTUAL_NETWORK_BASIC_ENDPOINT_H_

namespace virtual_network {

template <class Protocol>
class basic_VirtualLink_endpoint {
 public:
  typedef Protocol protocol_type;

  typedef typename protocol_type::endpoint_context_type internal_context_type;
  typedef typename protocol_type::next_layer_protocol::endpoint
      next_layer_endpoint_type;

 public:
  template <class... Args>
  basic_VirtualLink_endpoint(const Args&... args)
      : set_(!!sizeof...(Args)),
        internal_context_(),
        next_layer_endpoint_(args...) {}

  template <class... Args>
  basic_VirtualLink_endpoint(const internal_context_type& context,
                             Args&&... args)
      : set_(!!sizeof...(Args)),
        internal_context_(context),
        next_layer_endpoint_(std::forward<Args>(args)...) {}

  basic_VirtualLink_endpoint(const basic_VirtualLink_endpoint& other)
      : set_(other.set_),
        internal_context_(other.internal_context_),
        next_layer_endpoint_(other.next_layer_endpoint_) {}

  basic_VirtualLink_endpoint& operator=(
      const basic_VirtualLink_endpoint& endpoint) {
    set_ = endpoint.set_;
    internal_context_ = endpoint.internal_context_;
    next_layer_endpoint_ = endpoint.next_layer_endpoint_;

    return *this;
  }

  bool operator==(const basic_VirtualLink_endpoint& rhs) const {
    if (set_ != rhs.set_) {
      return false;
    }

    if (!set_) {
      return true;
    }

    return (internal_context_ == rhs.internal_context_) &&
           (next_layer_endpoint_ == rhs.next_layer_endpoint_);
  }

  bool operator<(const basic_VirtualLink_endpoint& rhs) const {
    if (set_ != rhs.set_) {
      return !set_;
    }

    if (!set_) {
      return false;
    }

    if (internal_context_ != rhs.internal_context_) {
      return internal_context_ < rhs.internal_context_;
    }

    return next_layer_endpoint_ < rhs.next_layer_endpoint_;
  }

  bool is_set() const { return set_; }

  void set() { set_ = true; }

  internal_context_type& endpoint_context() { return internal_context_; }

  const internal_context_type& endpoint_context() const {
    return internal_context_;
  }

  next_layer_endpoint_type& next_layer_endpoint() { return next_layer_endpoint_; }

  const next_layer_endpoint_type& next_layer_endpoint() const {
    return next_layer_endpoint_;
  }

  protocol_type protocol() const { return protocol_type(); }

 private:
  bool set_;
  internal_context_type internal_context_;
  next_layer_endpoint_type next_layer_endpoint_;
};

}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_BASIC_ENDPOINT_H_
