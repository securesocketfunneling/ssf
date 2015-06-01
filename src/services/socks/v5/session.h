#ifndef SRC_SERVICES_SOCKS_V5_SESSION_H_
#define SRC_SERVICES_SOCKS_V5_SESSION_H_

#include <memory>

#include <boost/noncopyable.hpp>  // NOLINT
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>  // NOLINT

#include "common/network/base_session.h"  // NOLINT
#include "common/network/socket_link.h"  // NOLINT

#include "common/network/manager.h"
#include "common/network/base_session.h"

#include "services/socks/v5/request.h"  // NOLINT
#include "services/socks/v5/request_auth.h"  // NOLINT

#include "common/boost/fiber/stream_fiber.hpp"

namespace ssf { namespace socks { namespace v5 {

template <typename Demux>
class Session : public ssf::BaseSession {
private:
  typedef std::array<char, 50 * 1024> StreamBuff;

  typedef boost::asio::ip::tcp::socket socket;
  typedef typename boost::asio::fiber::stream_fiber<
      typename Demux::socket_type>::socket fiber;

  typedef ItemManager<BaseSessionPtr> SessionManager;

 public:
   Session(SessionManager* sm, fiber client);

 public:
   virtual void start(boost::system::error_code&);

   virtual void stop(boost::system::error_code&);

 private:
  void HandleRequestAuthDispatch(const boost::system::error_code& ec, std::size_t);

  void DoNoAuth();
  void DoErrorAuth();

  void HandleReplyAuthSent();

  void HandleRequestDispatch(const boost::system::error_code&, std::size_t);

  void DoConnectRequest();

  void DoBindRequest();

  void DoUDPRequest();

  void HandleApplicationServerConnect(const boost::system::error_code&);

  void EstablishLink();

  void HandleStop();

 private:
  std::shared_ptr<Session> self_shared_from_this() {
    return std::static_pointer_cast<Session>(shared_from_this());
  }


 private:
  boost::asio::io_service& io_service_;
  SessionManager* p_session_manager_;

  fiber client_;
  socket app_server_;
  RequestAuth request_auth_;
  Request request_;

  std::shared_ptr<StreamBuff> upstream_;
  std::shared_ptr<StreamBuff> downstream_;
};
}  // v5
}  // socks
}  // ssf

#include "services/socks/v5/session.ipp"

#endif  // SRC_SERVICES_SOCKS_V5_SESSION_H_
