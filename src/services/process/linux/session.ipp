#ifndef SSF_SERVICES_PROCESS_SESSION_IPP_
#define SSF_SERVICES_PROCESS_SESSION_IPP_

#include <signal.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <ssf/log/log.h>
#include <ssf/network/session_forwarder.h>
#include <ssf/network/socket_link.h>

#include "common/error/error.h"

#define SSF_PIPE_READ 0
#define SSF_PIPE_WRITE 1
#define INVALID_PIPE_ID -1
#define INVALID_CHILD_PID -1

namespace ssf {
namespace services {
namespace process {
namespace linux {

template <typename Demux>
Session<Demux>::Session(SessionManager *p_session_manager, fiber client)
    : ssf::BaseSession(),
      io_service_(client.get_io_service()),
      p_session_manager_(p_session_manager),
      client_(std::move(client)),
      child_pid_(-1),
      master_tty_(-1),
      sd_(io_service_) {}

template <typename Demux>
void Session<Demux>::start(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "session[process]: start";
  int master_tty;
  int slave_tty;

  InitMasterSlaveTty(&master_tty, &slave_tty, ec);

  if (ec) {
    SSF_LOG(kLogError) << "session[process]: init tty failed";
    stop(ec);
    return;
  }

  child_pid_ = fork();
  switch(child_pid_) {
    case -1:
      SSF_LOG(kLogError) << "session[process]: fork failed";
      ec.assign(::error::process_not_created, ::error::get_ssf_category());
      stop(ec);
      return;
    case 0:
      // child

      struct termios slave_orig_term_settings;
      struct termios new_term_settings;

      close(master_tty);

      tcgetattr(slave_tty, &slave_orig_term_settings);

      new_term_settings = slave_orig_term_settings;
      cfmakeraw (&new_term_settings);
      tcsetattr (slave_tty, TCSANOW, &new_term_settings);

      setsid();
      ioctl(0, TIOCSCTTY, NULL);

      while((dup2(slave_tty, STDOUT_FILENO) == -1) && (errno == EINTR)) {}
      while((dup2(slave_tty, STDERR_FILENO) == -1) && (errno == EINTR)) {}
      while((dup2(slave_tty, STDIN_FILENO) == -1) && (errno == EINTR)) {}

      if (execl("/bin/bash", "/bin/bash") == -1) {
        exit(1);
      }
      exit(0);
    default:
      break;
  };

  master_tty_ = master_tty;

  StartForwarding(ec);
  if (ec) {
    stop(ec);
  }
}

template <typename Demux>
void Session<Demux>::stop(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "session[process]: stop";
  client_.close();
  if (ec) {
    SSF_LOG(kLogError) << "session[process]: stop error " << ec.message();
  }

  if (child_pid_ != INVALID_PIPE_ID) {
    kill(child_pid_, SIGTERM);
  }

  if (master_tty_ != INVALID_PIPE_ID) {
    close(master_tty_);
  }

  sd_.close(ec);
}

template <typename Demux>
void Session<Demux>::InitMasterSlaveTty(int* p_master_tty, int* p_slave_tty,
                                        boost::system::error_code& ec) {
  *p_master_tty = posix_openpt(O_RDWR | O_NOCTTY);
  if (*p_master_tty < 0) {
    SSF_LOG(kLogError) << "session[process]: could not open master tty";
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    return;
  }

  if (grantpt(*p_master_tty) != 0) {
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    return;
  }
  if (unlockpt(*p_master_tty) != 0) {
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    return;
  }

  *p_slave_tty = open(ptsname(*p_master_tty), O_RDWR | O_NOCTTY);
  if (*p_slave_tty < 0) {
    SSF_LOG(kLogError) << "session[process]: could not open slave tty";
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    return;
  }
}

template <typename Demux>
void Session<Demux>::StartForwarding(boost::system::error_code& ec) {
  sd_.assign(master_tty_, ec);
  if (ec) {
    SSF_LOG(kLogError)
        << "session[process]: could not initialize stream handle";
    return;
  }

  // pipe process stdout/stderr to socket output
  AsyncEstablishHDLink(ReadFrom(sd_), WriteTo(client_),
                       boost::asio::buffer(downstream_),
                       Then(&Session::StopHandler, this->SelfFromThis()));
  // pipe socket input to process stdin
  AsyncEstablishHDLink(ReadFrom(client_), WriteTo(sd_),
                       boost::asio::buffer(upstream_),
                       Then(&Session::StopHandler, this->SelfFromThis()));
}

}  // linux
}  // process
}  // services
}  // ssf

#endif  // SSF_SERVICES_PROCESS_SESSION_IPP_
