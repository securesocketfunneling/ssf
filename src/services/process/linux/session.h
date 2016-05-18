#ifndef SSF_SERVICES_PROCESS_SESSION_H_
#define SSF_SERVICES_PROCESS_SESSION_H_

#include <unistd.h>
#include <sys/wait.h>
#include <memory>
#include <map>

#include <boost/noncopyable.hpp>  // NOLINT
#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>  // NOLINT
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/signal_set.hpp>

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

  enum {
    kInvalidProcessId = -1,
    kInvalidTtyDescriptor = -1
  };

 public:
  Session(SessionManager* sm, fiber client,
          const std::string& binary_path);

 public:
  void start(boost::system::error_code&) override;

  void stop(boost::system::error_code&) override;

 private:
  std::shared_ptr<Session> SelfFromThis() {
    return std::static_pointer_cast<Session>(this->shared_from_this());
  }

  void InitPipes(int pipe_out[2], int pipe_err[2], int pipe_in[2],
                 boost::system::error_code& ec);

  void InitMasterSlaveTty(int* p_master, int* p_slave,
                          boost::system::error_code& ec);

  void StartForwarding(boost::system::error_code& ec);

  void StopHandler(const boost::system::error_code& ec);
  
  void StartSignalWait();
  
  void SigchldHandler(const boost::system::error_code& ec,
                      int sig_num);
  
 private:
  boost::asio::io_service& io_service_;
  SessionManager* p_session_manager_;

  fiber client_;
  boost::asio::signal_set signal_;

  std::string binary_path_;
  pid_t child_pid_;

  int master_tty_;

  boost::asio::posix::stream_descriptor sd_;

  StreamBuff upstream_;
  StreamBuff downstream_;
};

}  // linux
}  // process
}  // services
}  // ssf

#include "services/process/linux/session.ipp"

#endif  // SSF_SERVICES_PROCESS_SESSION_H_
