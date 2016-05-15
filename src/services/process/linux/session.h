#ifndef SSF_SERVICES_PROCESS_SESSION_H_
#define SSF_SERVICES_PROCESS_SESSION_H_

#include <memory>

#include <boost/noncopyable.hpp>  // NOLINT
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>  // NOLINT

#include <ssf/network/base_session.h>  // NOLINT
#include <ssf/network/socket_link.h>   // NOLINT
#include <ssf/network/manager.h>
#include <ssf/network/base_session.h>

#include "services/socks/v4/request.h"  // NOLINT

#include "common/boost/fiber/stream_fiber.hpp"

namespace ssf {
namespace services {
namespace process {

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
  void start(boost::system::error_code&) override;

  void stop(boost::system::error_code&) override;

 private:
  std::shared_ptr<Session> SelfFromThis() {
    return std::static_pointer_cast<Session>(this->shared_from_this());
  }

 private:
  boost::asio::io_service& io_service_;
  SessionManager* p_session_manager_;

  fiber client_;

  std::shared_ptr<StreamBuff> upstream_;
  std::shared_ptr<StreamBuff> downstream_;
};

}  // process
}  // services
}  // ssf

#include "services/process/session.ipp"

#endif  // SSF_SERVICES_PROCESS_SESSION_H_