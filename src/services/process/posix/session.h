#ifndef SSF_SERVICES_PROCESS_POSIX_SESSION_H_
#define SSF_SERVICES_PROCESS_POSIX_SESSION_H_

#include <sys/types.h>

#include <array>
#include <memory>

#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/signal_set.hpp>

#include <ssf/network/base_session.h>
#include <ssf/network/socket_link.h>
#include <ssf/network/manager.h>
#include <ssf/network/base_session.h>

#include "common/boost/fiber/stream_fiber.hpp"

namespace ssf {
namespace services {
namespace process {

template<typename Demux>
class Server;

namespace posix {

template <typename Demux>
class Session : public ssf::BaseSession {
 private:
  using StreamBuf = std::array<char, 50 * 1024>;
  using Fiber = typename boost::asio::fiber::stream_fiber<
      typename Demux::socket_type>::socket;
  using ShellServer = Server<Demux>;

  enum { kInvalidProcessId = -1, kInvalidTtyDescriptor = -1 };

 public:
  Session(std::weak_ptr<ShellServer> server, Fiber client,
          const std::string& binary_path, const std::string& binary_args);

  ~Session();

 public:
  void start(boost::system::error_code&) override;

  void stop(boost::system::error_code&) override;

 private:
  std::shared_ptr<Session> SelfFromThis();

  static void ChdirHome(boost::system::error_code& ec);

  static void GenerateArgv(const std::string& binary_name,
                           const std::list<std::string>& splitted_argv,
                           std::vector<char*>& argv);

  void InitMasterSlaveTty(int* p_master, int* p_slave,
                          boost::system::error_code& ec);

  void StartForwarding(boost::system::error_code& ec);

  void StopHandler(const boost::system::error_code& ec);

  void StartSignalWait();

  void SigchldHandler(const boost::system::error_code& ec, int sig_num);

 private:
  boost::asio::io_service& io_service_;
  std::weak_ptr<ShellServer> p_server_;

  Fiber client_;
  boost::asio::signal_set signal_;

  std::string binary_path_;
  std::string binary_args_;

  pid_t child_pid_;

  int master_tty_;

  boost::asio::posix::stream_descriptor sd_;

  StreamBuf upstream_;
  StreamBuf downstream_;
};

}  // posix
}  // process
}  // services
}  // ssf

#include "services/process/posix/session.ipp"

#endif  // SSF_SERVICES_PROCESS_POSIX_SESSION_H_
