#ifndef SSF_SERVICES_FIBERS_TO_SOCKETS_SESSION_H_
#define SSF_SERVICES_FIBERS_TO_SOCKETS_SESSION_H_

#include <array>
#include <memory>

#include <boost/system/error_code.hpp>

#include <boost/asio/socket_base.hpp>

#include <ssf/log/log.h>

#include "ssf/network/base_session.h"  // NOLINT
#include "ssf/network/socket_link.h"

namespace ssf {
namespace services {
namespace fibers_to_sockets {

/// Create a Full Duplex Forwarding Link
template <typename Demux, typename InwardStream, typename ForwardStream>
class Session : public ssf::BaseSession {
 private:
  /// Buffer type for the transiting data
  using StreamBuf = std::array<char, 50 * 1024>;

 public:
  using Server = FibersToSockets<Demux>;
  using FibersToSocketsWPtr = std::weak_ptr<Server>;

 public:
  using SessionPtr = std::shared_ptr<Session>;

 public:
  /// Return a shared pointer to a new SessionForwarder object
  template <typename... Args>
  static SessionPtr create(Args&&... args) {
    return SessionPtr(new Session(std::forward<Args>(args)...));
  }

  ~Session() {}

  /// Start forwarding
  void start(boost::system::error_code&) override {
    SSF_LOG(kLogDebug) << "session[stream_forwarder]: start";
    DoForward();
  }

  /// Stop forwarding
  void stop(boost::system::error_code&) override {
    SSF_LOG(kLogDebug) << "session[stream_forwarder]: stop";
    boost::system::error_code ec;
    if (inbound_.lowest_layer().is_open()) {
      inbound_.lowest_layer().shutdown(boost::asio::socket_base::shutdown_both,
                                       ec);
      inbound_.lowest_layer().close(ec);
    }

    if (outbound_.lowest_layer().is_open()) {
      outbound_.lowest_layer().shutdown(boost::asio::socket_base::shutdown_both,
                                        ec);
      outbound_.lowest_layer().close(ec);
    }
  }

 private:
  /// Function taking a member function and returning a handler
  /**
  * Function taking a memeber function and returning a handler including
  * reference counting functionality.
  *
  * @param handler A member function with one argument.
  */

  /// It is needed to cast the shared pointer from shared_from_this because it
  /// is the base class which inherit from enable_shared_from_this
  std::shared_ptr<Session> SelfFromThis() {
    return std::static_pointer_cast<Session>(this->shared_from_this());
  }

 private:
  /// The constructor is made private to ensure users only use create()
  Session(FibersToSocketsWPtr server, InwardStream inbound,
          ForwardStream outbound)
      : server_(server),
        inbound_(std::move(inbound)),
        outbound_(std::move(outbound)) {}

  /// Start forwarding
  void DoForward() {
    // Make two Half Duplex links to have a Full Duplex Link
    AsyncEstablishHDLink(
        ReadFrom(inbound_), WriteTo(outbound_),
        boost::asio::buffer(inwardBuffer_),
        std::bind(&Session::StopHandler, this->SelfFromThis(), std::placeholders::_1));

    AsyncEstablishHDLink(ReadFrom(outbound_), WriteTo(inbound_),
                         boost::asio::buffer(forwardBuffer_),
                         std::bind(&Session::StopHandler, this->SelfFromThis(),
                                   std::placeholders::_1));
  }

  /// Stop forwarding
  void StopHandler(const boost::system::error_code& ec) {
    boost::system::error_code e;
    if (auto p_server = server_.lock()) {
      p_server->StopSession(this->SelfFromThis(), e);
    }
  }

 private:
  FibersToSocketsWPtr server_;

  // The streams to forward to each other
  InwardStream inbound_;
  ForwardStream outbound_;

  // One buffer for each Half Duplex Link
  StreamBuf inwardBuffer_;
  StreamBuf forwardBuffer_;
};

}  // fibers_to_sockets
}  // services
}  // ssf

#endif  // SSF_SERVICES_FIBERS_TO_SOCKETS_SESSION_H_
