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
Session<Demux>::Session(SessionManager *p_session_manager, fiber client,
                        const std::string& binary_path)
    : ssf::BaseSession(),
      io_service_(client.get_io_service()),
      p_session_manager_(p_session_manager),
      client_(std::move(client)),
      signal_(io_service_, SIGCHLD),
      binary_path_(binary_path),
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
  if (child_pid_ < 0) {
    SSF_LOG(kLogError) << "session[process]: fork failed";
    ec.assign(::error::process_not_created, ::error::get_ssf_category());
    stop(ec);
    return;
  }
  
  if (child_pid_ == 0) {
    // child
    struct termios new_term_settings;
    close(master_tty);

    tcgetattr(slave_tty, &new_term_settings);

    // set tty as raw (input char by char, echoing disabled, special process
    // for input/output characters disabled.
    cfmakeraw (&new_term_settings);
    tcsetattr (slave_tty, TCSANOW, &new_term_settings);

    // new process as session leader
    setsid();
    // slave side as controlling terminal
    ioctl(0, TIOCSCTTY, 1);

    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    close(STDIN_FILENO);
      
    while((dup2(slave_tty, STDOUT_FILENO) == -1) && (errno == EINTR)) {}
    while((dup2(slave_tty, STDERR_FILENO) == -1) && (errno == EINTR)) {}
    while((dup2(slave_tty, STDIN_FILENO) == -1) && (errno == EINTR)) {}

    // dup2 done, clode descriptor
    close(slave_tty);

    std::size_t slash_pos = binary_path_.rfind('/');
    std::string binary_name = std::string::npos != slash_pos ?
      binary_path_.substr(slash_pos + 1) : binary_path_;

    if (execl(binary_path_.c_str(), binary_name.c_str(), (char*) NULL) == -1) {
      exit(1);
    }
    exit(0);
  }

  // parent
  master_tty_ = master_tty;

  StartSignalWait();

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
    child_pid_ = INVALID_PIPE_ID;
  }
  
  
  if (master_tty_ != INVALID_PIPE_ID) {
    close(master_tty_);
    master_tty_ = INVALID_PIPE_ID;
  }

  sd_.close(ec);
  signal_.cancel(ec);
}

template <typename Demux>
void Session<Demux>::StopHandler(const boost::system::error_code& ec) {
  boost::system::error_code e;
  p_session_manager_->stop(this->SelfFromThis(), e);
}



template <typename Demux>
void Session<Demux>::StartSignalWait() {
  signal_.async_wait(boost::bind(&Session::SigchldHandler, this->SelfFromThis(),
                     _1, _2));
}

template <typename Demux>
void Session<Demux>::SigchldHandler(const boost::system::error_code& ec,
                                    int sig_num) {
  if (ec) {
    return;
  }
  
  int status;
  if (waitpid(child_pid_, &status, WNOHANG) <= 0) {
    // child state did not change
    StartSignalWait();
    return;
  }
  
  if (!WIFEXITED(status) && !WIFSIGNALED(status)) {
    // child did not terminate
    SSF_LOG(kLogInfo) << "session[process]: child " << child_pid_
                      << " state changed";
    StartSignalWait();
    return;
  }
    
  // child terminated, close session
  SSF_LOG(kLogInfo) << "session[process]: child " << child_pid_
                    << " terminated";
  child_pid_ = INVALID_PIPE_ID;
  StopHandler(ec);
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
                       boost::bind(&Session::StopHandler, this->SelfFromThis(),
                                   _1));
  // pipe socket input to process stdin
  AsyncEstablishHDLink(ReadFrom(client_), WriteTo(sd_),
                       boost::asio::buffer(upstream_),
                       boost::bind(&Session::StopHandler, this->SelfFromThis(),
                                   _1));
}

}  // linux
}  // process
}  // services
}  // ssf

#endif  // SSF_SERVICES_PROCESS_SESSION_IPP_
