#ifndef SSF_COMMON_NETWORK_NETWORK_POLICY_TRAITS_H_
#define SSF_COMMON_NETWORK_NETWORK_POLICY_TRAITS_H_

namespace ssf {
// Traits defining the return types of layer policies

template <typename Policy>
struct SocketOf {
  typedef typename Policy::socket_type type;
};

template <typename Policy>
struct SocketPtrOf {
  typedef typename Policy::p_socket_type type;
};

template <typename Policy>
struct AcceptorOf {
  typedef typename Policy::acceptor_type type;
};

template <typename Policy>
struct AcceptorPtrOf {
  typedef typename Policy::p_acceptor_type type;
};
}

#endif  // SSF_COMMON_NETWORK_NETWORK_POLICY_TRAITS_H_