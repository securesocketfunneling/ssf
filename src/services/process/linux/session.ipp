#ifndef SSF_SERVICES_PROCESS_SESSION_IPP_
#define SSF_SERVICES_PROCESS_SESSION_IPP_

#include <signal.h>
#include <sys/types.h>
#include <stdlib.h>
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
      pipe_out_(INVALID_PIPE_ID),
      pipe_err_(INVALID_PIPE_ID),
      pipe_in_(INVALID_PIPE_ID),
      sd_out_(io_service_),
      sd_err_(io_service_),
      sd_in_(io_service_) {}

template <typename Demux>
void Session<Demux>::start(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "session[process]: start";
  int pipe_out[2];
  int pipe_err[2];
  int pipe_in[2];

  InitPipes(pipe_out, pipe_err, pipe_in, ec);
  if (ec) {
    SSF_LOG(kLogError) << "session[process]: init pipes failed";
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

      // redirect standard io in pies
      while((dup2(pipe_out[SSF_PIPE_WRITE], STDOUT_FILENO) == -1) && (errno == EINTR)) {}
      while((dup2(pipe_err[SSF_PIPE_WRITE], STDERR_FILENO) == -1) && (errno == EINTR)) {}
      while((dup2(pipe_in[SSF_PIPE_READ], STDIN_FILENO) == -1) && (errno == EINTR)) {}

      close(pipe_out[SSF_PIPE_READ]);
      close(pipe_out[SSF_PIPE_WRITE]);
      close(pipe_err[SSF_PIPE_READ]);
      close(pipe_err[SSF_PIPE_WRITE]);
      close(pipe_in[SSF_PIPE_READ]);
      close(pipe_in[SSF_PIPE_WRITE]);
      if (execl("/bin/bash", "/bin/bash", "-x") == -1) {
        exit(1);
      }
    default:
      break;
  };
  close(pipe_out[SSF_PIPE_WRITE]);
  close(pipe_err[SSF_PIPE_WRITE]);
  close(pipe_in[SSF_PIPE_READ]);
  pipe_out_ = pipe_out[SSF_PIPE_READ];
  pipe_err_ = pipe_err[SSF_PIPE_READ];
  pipe_in_ = pipe_in[SSF_PIPE_WRITE];
  StartForwarding(ec);
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

  if (pipe_out_ != INVALID_PIPE_ID) {
    close(pipe_out_);
  }
  if (pipe_err_ != INVALID_PIPE_ID) {
    close(pipe_err_);
  }
  if (pipe_in_ != INVALID_PIPE_ID) {
    close(pipe_in_);
  }

  sd_out_.close(ec);
  sd_err_.close(ec);
  sd_in_.close(ec);
}


template <typename Demux>
void Session<Demux>::StartForwarding(boost::system::error_code& ec) {
  sd_out_.assign(pipe_out_, ec);
  if (ec) {
    SSF_LOG(kLogError)
        << "session[process]: could not initialize out stream handle";
    return;
  }
  pipe_out_ = INVALID_PIPE_ID;
  sd_err_.assign(pipe_err_, ec);
  if (ec) {
    SSF_LOG(kLogError)
        << "session[process]: could not initialize err stream handle";
    return;
  }
  pipe_err_ = INVALID_PIPE_ID;
  sd_in_.assign(pipe_in_, ec);
  if (ec) {
    SSF_LOG(kLogError)
        << "session[process]: could not initialize in stream handle";
    return;
  }
  pipe_in_ = INVALID_PIPE_ID;

  // pipe process stdout to socket output
  AsyncEstablishHDLink(ReadFrom(sd_out_), WriteTo(client_),
                       boost::asio::buffer(downstream_out_),
                       Then(&Session::StopHandler, this->SelfFromThis()));
  // pipe process stderr to socket output
  AsyncEstablishHDLink(ReadFrom(sd_err_), WriteTo(client_),
                       boost::asio::buffer(downstream_err_),
                       Then(&Session::StopHandler, this->SelfFromThis()));
  // pipe socket input to process stdin
  AsyncEstablishHDLink(ReadFrom(client_), WriteTo(sd_in_),
                       boost::asio::buffer(upstream_),
                       Then(&Session::StopHandler, this->SelfFromThis()));
}


template <typename Demux>
void Session<Demux>::InitPipes(int pipe_out[2], int pipe_err[2], int pipe_in[2],
                               boost::system::error_code& ec) {
  if (pipe(pipe_out) == -1) {
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    return;
  }
  if (pipe(pipe_err) == -1) {
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    goto cleanup_pipe_out;
  }
  if (pipe(pipe_in) == -1) {
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    goto cleanup_pipe_err;
  }

  return;

cleanup_pipe_err:
  close(pipe_err[SSF_PIPE_READ]);
  close(pipe_err[SSF_PIPE_WRITE]);
cleanup_pipe_out:
  close(pipe_out[SSF_PIPE_READ]);
  close(pipe_out[SSF_PIPE_WRITE]);
}


}  // linux
}  // process
}  // services
}  // ssf

#endif  // SSF_SERVICES_PROCESS_SESSION_IPP_
