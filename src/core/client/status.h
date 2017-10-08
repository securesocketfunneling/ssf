#ifndef SSF_CORE_CLIENT_STATUS_H_
#define SSF_CORE_CLIENT_STATUS_H_

namespace ssf {

enum class Status {
  kInitialized,
  kEndpointNotResolvable,
  kServerUnreachable,
  kServerNotSupported,
  kDisconnected,
  kConnected,
  kRunning
};

}  // ssf

#endif  // SSF_CORE_CLIENT_STATUS_H_
