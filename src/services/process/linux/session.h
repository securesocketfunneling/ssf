#ifndef SSF_SERVICES_PROCESS_SESSION_H_
#define SSF_SERVICES_PROCESS_SESSION_H_

#include <sys/wait.h>
#include <memory>

#include <boost/noncopyable.hpp>  // NOLINT
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>  // NOLINT
#include <boost/asio/posix/stream_descriptor.hpp>

#include <ssf/network/base_session.h>  // NOLINT
#include <ssf/network/socket_link.h>   // NOLINT
#include <ssf/network/manager.h>
#include <ssf/network/base_session.h>

#include "services/socks/v4/request.h"  // NOLINT

#include "common/boost/fiber/stream_fiber.hpp"

namespace ssf {
namespace services {
namespace process {
namespace linux {

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

  template <typename Handler, typename This>
  auto Then(Handler handler,
            This me) -> decltype(boost::bind(handler, me->SelfFromThis(), _1)) {
    return boost::bind(handler, me->SelfFromThis(), _1);
  }

  void StopHandler(const boost::system::error_code& ec) {
    boost::system::error_code e;
    p_session_manager_->stop(this->SelfFromThis(), e);
  }

  void InitPipes(int pipe_out[2], int pipe_err[2], int pipe_in[2],
                 boost::system::error_code& ec);

  void StartForwarding(boost::system::error_code& ec);
 private:
  boost::asio::io_service& io_service_;
  SessionManager* p_session_manager_;

  fiber client_;

  pid_t child_pid_;
  int pipe_out_;
  int pipe_err_;
  int pipe_in_;

  boost::asio::posix::stream_descriptor sd_out_;
  boost::asio::posix::stream_descriptor sd_err_;
  boost::asio::posix::stream_descriptor sd_in_;

  StreamBuff upstream_;
  StreamBuff downstream_out_;
  StreamBuff downstream_err_;
};

}  // linux
}  // process
}  // services
}  // ssf

#include "services/process/linux/session.ipp"

#endif  // SSF_SERVICES_PROCESS_SESSION_H_
