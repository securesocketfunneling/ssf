#ifndef SSF_SERVICES_PROCESS_POSIX_SESSION_IPP_
#define SSF_SERVICES_PROCESS_POSIX_SESSION_IPP_

#include <signal.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <vector>

#include <boost/algorithm/string.hpp>

#include <ssf/log/log.h>
#include <ssf/network/session_forwarder.h>
#include <ssf/network/socket_link.h>

#include "common/error/error.h"

namespace ssf {
namespace services {
namespace process {
namespace posix {

template <typename Demux>
Session<Demux>::Session(SessionManager* p_session_manager, fiber client,
                        const std::string& binary_path,
                        const std::string& binary_args)
    : ssf::BaseSession(),
      io_service_(client.get_io_service()),
      p_session_manager_(p_session_manager),
      client_(std::move(client)),
      signal_(io_service_, SIGCHLD),
      binary_path_(binary_path),
      binary_args_(binary_args),
      child_pid_(kInvalidProcessId),
      master_tty_(kInvalidTtyDescriptor),
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

    // set tty as raw (input char by char, echoing disabled, special process
    // for input/output characters disabled.
    tcgetattr(slave_tty, &new_term_settings);
    cfmakeraw(&new_term_settings);
    tcsetattr(slave_tty, TCSANOW, &new_term_settings);

    // new process as session leader
    setsid();

    // slave side as controlling terminal
    ioctl(slave_tty, TIOCSCTTY, 1);

    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    close(STDIN_FILENO);

    while ((dup2(slave_tty, STDOUT_FILENO) == -1) && (errno == EINTR)) {
    }
    while ((dup2(slave_tty, STDERR_FILENO) == -1) && (errno == EINTR)) {
    }
    while ((dup2(slave_tty, STDIN_FILENO) == -1) && (errno == EINTR)) {
    }

    // dup2 done, close descriptor
    close(slave_tty);

    std::size_t slash_pos = binary_path_.rfind('/');
    std::string binary_name = std::string::npos != slash_pos
                                  ? binary_path_.substr(slash_pos + 1)
                                  : binary_path_;

    std::vector<char*> argv;
    std::list<std::string> splitted_args;
    if (binary_args_ != "") {
      boost::split(splitted_args, binary_args_, boost::algorithm::is_any_of(" "));
      argv.resize(splitted_args.size() + 2);
      std::size_t i = 1;
      for (auto& arg: splitted_args) {
        argv[i] = const_cast<char*>(arg.c_str());
        ++i;
      }
    } else {
      argv.resize(2);
    }

    argv[0] = const_cast<char*>(binary_name.c_str());
    argv[argv.size() - 1] = nullptr;

    execv(binary_path_.c_str(), argv.data());

    fprintf(stderr, "Exiting: fail to exec <%s>\n", binary_path_.c_str());
    exit(1);
  }

  // parent
  close(slave_tty);
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

  if (child_pid_ > 0) {
    kill(child_pid_, SIGTERM);
    child_pid_ = kInvalidProcessId;
  }

  if (master_tty_ > kInvalidTtyDescriptor) {
    close(master_tty_);
    master_tty_ = kInvalidTtyDescriptor;
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
  signal_.async_wait(
      boost::bind(&Session::SigchldHandler, this->SelfFromThis(), _1, _2));
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
    StartSignalWait();
    return;
  }

  // child terminated, close session
  child_pid_ = kInvalidProcessId;
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
  AsyncEstablishHDLink(
      ReadFrom(sd_), WriteTo(client_), boost::asio::buffer(downstream_),
      boost::bind(&Session::StopHandler, this->SelfFromThis(), _1));
  // pipe socket input to process stdin
  AsyncEstablishHDLink(
      ReadFrom(client_), WriteTo(sd_), boost::asio::buffer(upstream_),
      boost::bind(&Session::StopHandler, this->SelfFromThis(), _1));
}

}  // posix
}  // process
}  // services
}  // ssf

#endif  // SSF_SERVICES_PROCESS_POSIX_SESSION_IPP_
