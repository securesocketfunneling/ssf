#ifndef SSF_COMMON_NETWORK_SESSION_FORWARDER_H
#define SSF_COMMON_NETWORK_SESSION_FORWARDER_H

#include <array>
#include <memory>

#include <boost/system/error_code.hpp>
#include <boost/asio/ssl.hpp>

#include "common/network/base_session.h"  // NOLINT
#include "common/network/manager.h"
#include "common/network/socket_link.h"

namespace ssf {

/// Create a Full Duplex Forwarding Link
template <typename InwardStream, typename ForwardStream>
class SessionForwarder : public ssf::BaseSession {
private:
  /// Buffer type for the transiting data
  typedef std::array<char, 50 * 1024> buffer_type;

  /// Type for the class managing the different forwarding links
  typedef ItemManager<BaseSessionPtr> SessionManager;

public:
  typedef std::shared_ptr<SessionForwarder> p_SessionForwarder;


public:
  /// Return a shared pointer to a new SessionForwarder object
  template <typename ... Args>
  static p_SessionForwarder create(Args&& ... args) {
    return std::shared_ptr<SessionForwarder>(
        new SessionForwarder(std::forward<Args>(args)...));
  }

  /// Start forwarding
  virtual void start(boost::system::error_code&) {
    DoForward();
  }

  /// Stop forwarding
  virtual void stop(boost::system::error_code&) {
    boost::system::error_code ec;
    inbound_.lowest_layer().shutdown(
        boost::asio::ip::tcp::socket::shutdown_both, ec);
    inbound_.lowest_layer().close();
    outbound_.lowest_layer().shutdown(
        boost::asio::ip::tcp::socket::shutdown_both, ec);
    outbound_.lowest_layer().close();
  }

private:
  /// Function taking a member function and returning a handler
  /**
  * Function taking a memeber function and returning a handler including 
  * reference counting functionality.
  * 
  * @param handler A member function with one argument.
  */

  //template <typename Handler, typename This>
  //auto Then(Handler handler, This me) 
  //  -> decltype(boost::bind(handler, me->SelfFromThis(), _1)) {
  //  return boost::bind(handler, me->SelfFromThis(), _1);
  //}

  template <typename Handler, typename This>
  auto Then(Handler handler, This me) -> decltype(boost::bind(handler,
                                        me->SelfFromThis(),
                                        _1)) {
    return boost::bind(handler, me->SelfFromThis(), _1);
  }

  /// It is needed to cast the shared pointer from shared_from_this because it
  /// is the base class which inherit from enable_shared_from_this
  std::shared_ptr<SessionForwarder> SelfFromThis() {
    return std::static_pointer_cast<SessionForwarder>(this->shared_from_this());
  }

private:
 /// The constructor is made private to ensure users only use create()
 SessionForwarder(SessionManager* p_manager, InwardStream inbound,
                  ForwardStream outbound)
     : inbound_(std::move(inbound)),
       outbound_(std::move(outbound)),
       p_manager_(p_manager) {}
  
  /// Start forwarding
  void DoForward() {
    // Make two Half Duplex links to have a Full Duplex Link
    AsyncEstablishHDLink(ReadFrom(inbound_), WriteTo(outbound_),
      boost::asio::buffer(inwardBuffer_),
      Then(&SessionForwarder::StopHandler, this->SelfFromThis()));

    AsyncEstablishHDLink(ReadFrom(outbound_), WriteTo(inbound_),
      boost::asio::buffer(forwardBuffer_),
      Then(&SessionForwarder::StopHandler, this->SelfFromThis()));
  }

  /// Stop forwarding
  void StopHandler(const boost::system::error_code& ec) {
    boost::system::error_code e;
    p_manager_->stop(SelfFromThis(), e);
  }

private:
  // The streams to forward to each other
  InwardStream inbound_;
  ForwardStream outbound_;

  /// The manager handling multiple SessionForwarder
  SessionManager* p_manager_;

  // One buffer for each Half Duplex Link
  buffer_type inwardBuffer_;
  buffer_type forwardBuffer_;
  
};
}  // ssf

#endif  // SSF_COMMON_NETWORK_SESSION_FORWARDER_H
