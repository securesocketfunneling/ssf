#ifndef SSF_SERVICES_PROCESS_WINDOWS_SESSION_H_
#define SSF_SERVICES_PROCESS_WINDOWS_SESSION_H_

#include <array>
#include <memory>

#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>

#include <windows.h>

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

namespace windows {

template <typename Demux>
class Session : public ssf::BaseSession {
 private:
  using StreamBuf = std::array<char, 50 * 1024>;
  using Fiber = typename boost::asio::fiber::stream_fiber<
      typename Demux::socket_type>::socket;
  using StreamHandle = boost::asio::windows::stream_handle;
  using ShellServer = typename Server<Demux>;

 public:
  Session(std::weak_ptr<ShellServer> server, Fiber client,
          const std::string& binary_path, const std::string& binary_args);

 public:
  void start(boost::system::error_code&) override;

  void stop(boost::system::error_code&) override;

 private:
  void StartForwarding(boost::system::error_code& ec);
  void StartProcess(boost::system::error_code& ec);
  void InitPipes(boost::system::error_code& ec);

  static void InitOutNamedPipe(const std::string& pipe_name,
                               HANDLE* p_read_pipe, HANDLE* p_write_pipe,
                               SECURITY_ATTRIBUTES* p_pipe_attributes,
                               DWORD pipe_size, boost::system::error_code& ec);

  static void InitInNamedPipe(const std::string& pipe_name, HANDLE* p_read_pipe,
                              HANDLE* p_write_pipe,
                              SECURITY_ATTRIBUTES* p_pipe_attributes,
                              DWORD pipe_size, boost::system::error_code& ec);

  std::shared_ptr<Session> SelfFromThis();

  void StopHandler(const boost::system::error_code& ec);

 private:
  boost::asio::io_service& io_service_;
  std::weak_ptr<ShellServer> p_server_;

  Fiber client_;

  std::string binary_path_;
  std::string binary_args_;

  std::string out_pipe_name_;
  std::string err_pipe_name_;
  std::string in_pipe_name_;

  PROCESS_INFORMATION process_info_;
  HANDLE data_out_;
  HANDLE data_err_;
  HANDLE data_in_;
  HANDLE proc_out_;
  HANDLE proc_err_;
  HANDLE proc_in_;
  HANDLE job_;

  StreamHandle h_out_;
  StreamHandle h_err_;
  StreamHandle h_in_;

  StreamBuf upstream_;
  StreamBuf downstream_out_;
  StreamBuf downstream_err_;
};

}  // windows
}  // process
}  // services
}  // ssf

#include "services/process/windows/session.ipp"

#endif  // SSF_SERVICES_PROCESS_WINDOWS_SESSION_H_