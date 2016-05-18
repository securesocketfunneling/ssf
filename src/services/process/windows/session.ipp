#ifndef SSF_SERVICES_PROCESS_WINDOWS_SESSION_IPP_
#define SSF_SERVICES_PROCESS_WINDOWS_SESSION_IPP_

#include <sstream>

#include <wincon.h>

#include <ssf/log/log.h>
#include <ssf/network/session_forwarder.h>
#include <ssf/network/socket_link.h>

#include "common/error/error.h"

namespace ssf {
namespace services {
namespace process {
namespace windows {

template <typename Demux>
Session<Demux>::Session(SessionManager* p_session_manager, fiber client,
                        const std::string& binary_path)
    : ssf::BaseSession(),
      io_service_(client.get_io_service()),
      p_session_manager_(p_session_manager),
      client_(std::move(client)),
      binary_path_(binary_path),
      out_pipe_name_("\\\\.\\pipe\\ssf_out_pipe_"),
      err_pipe_name_("\\\\.\\pipe\\ssf_err_pipe_"),
      in_pipe_name_("\\\\.\\pipe\\ssf_in_pipe_"),
      data_in_(INVALID_HANDLE_VALUE),
      data_out_(INVALID_HANDLE_VALUE),
      data_err_(INVALID_HANDLE_VALUE),
      proc_in_(INVALID_HANDLE_VALUE),
      proc_out_(INVALID_HANDLE_VALUE),
      proc_err_(INVALID_HANDLE_VALUE),
      h_out_(client_.get_io_service()),
      h_err_(client_.get_io_service()),
      h_in_(client_.get_io_service()) {
  memset(&process_info_, 0, sizeof(PROCESS_INFORMATION));
  process_info_.hProcess = INVALID_HANDLE_VALUE;
  process_info_.hThread = INVALID_HANDLE_VALUE;
}

template <typename Demux>
void Session<Demux>::start(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "session[process]: start";

  InitPipes(ec);
  if (ec) {
    SSF_LOG(kLogError) << "session[process]: pipes initialization failed";
    stop(ec);
    return;
  }

  StartProcess(ec);
  if (ec) {
    SSF_LOG(kLogError) << "session[process]: start process failed";
    stop(ec);
    return;
  }

  StartForwarding(ec);
  if (ec) {
    SSF_LOG(kLogError)
        << "session[process]: forwarding data from process to client failed";
    stop(ec);
    return;
  }
}

template <typename Demux>
void Session<Demux>::stop(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "session[process]: stop";

  if (process_info_.hProcess != INVALID_HANDLE_VALUE) {
    ::TerminateProcess(process_info_.hProcess, 0);
  }

  if (process_info_.hThread != INVALID_HANDLE_VALUE) {
    ::CloseHandle(process_info_.hThread);
  }
  if (process_info_.hProcess != INVALID_HANDLE_VALUE) {
    ::CloseHandle(process_info_.hProcess);
  }

  if (proc_in_ == INVALID_HANDLE_VALUE) {
    ::CloseHandle(proc_in_);
  }
  if (proc_out_ == INVALID_HANDLE_VALUE) {
    ::CloseHandle(proc_out_);
  }
  if (proc_err_ == INVALID_HANDLE_VALUE) {
    ::CloseHandle(proc_err_);
  }
  if (data_in_ == INVALID_HANDLE_VALUE) {
    ::CloseHandle(data_in_);
  }
  if (data_out_ == INVALID_HANDLE_VALUE) {
    ::CloseHandle(data_out_);
  }
  if (data_err_ == INVALID_HANDLE_VALUE) {
    ::CloseHandle(data_err_);
  }

  h_out_.close(ec);
  h_err_.close(ec);
  h_in_.close(ec);

  client_.close();

  if (ec) {
    SSF_LOG(kLogError) << "session[process]: stop error " << ec.message();
  }
}

template <typename Demux>
void Session<Demux>::StopHandler(const boost::system::error_code& ec) {
  boost::system::error_code e;
  p_session_manager_->stop(this->SelfFromThis(), e);
}

template <typename Demux>
std::shared_ptr<Session<Demux>> Session<Demux>::SelfFromThis() {
  return std::static_pointer_cast<Session>(this->shared_from_this());
}

template <typename Demux>
void Session<Demux>::StartForwarding(boost::system::error_code& ec) {
  h_out_.assign(data_out_, ec);
  if (ec) {
    SSF_LOG(kLogError)
        << "session[process]: could not initialize out stream handle";
    return;
  }
  data_out_ = INVALID_HANDLE_VALUE;
  h_err_.assign(data_err_, ec);
  if (ec) {
    SSF_LOG(kLogError)
        << "session[process]: could not initialize err stream handle";
    return;
  }
  data_err_ = INVALID_HANDLE_VALUE;
  h_in_.assign(data_in_, ec);
  if (ec) {
    SSF_LOG(kLogError)
        << "session[process]: could not initialize in stream handle";
    return;
  }
  data_in_ = INVALID_HANDLE_VALUE;

  // pipe process stdout to socket output
  AsyncEstablishHDLink(
      ReadFrom(h_out_), WriteTo(client_), boost::asio::buffer(downstream_out_),
      boost::bind(&Session::StopHandler, this->SelfFromThis(), _1));
  // pipe process stderr to socket output
  AsyncEstablishHDLink(
      ReadFrom(h_err_), WriteTo(client_), boost::asio::buffer(downstream_err_),
      boost::bind(&Session::StopHandler, this->SelfFromThis(), _1));
  // pipe socket input to process stdin
  AsyncEstablishHDLink(
      ReadFrom(client_), WriteTo(h_in_), boost::asio::buffer(upstream_),
      boost::bind(&Session::StopHandler, this->SelfFromThis(), _1));
}

template <typename Demux>
void Session<Demux>::StartProcess(boost::system::error_code& ec) {
  memset(&process_info_, 0, sizeof(PROCESS_INFORMATION));
  STARTUPINFO startup_info;

  ZeroMemory(&startup_info, sizeof(STARTUPINFO));
  startup_info.cb = sizeof(STARTUPINFO);
  startup_info.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
  startup_info.wShowWindow = SW_HIDE;
  startup_info.hStdOutput = proc_out_;
  startup_info.hStdError = proc_err_;
  startup_info.hStdInput = proc_in_;

  if (!::CreateProcessA(NULL, const_cast<char*>(binary_path_.c_str()), NULL,
                        NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL,
                        &startup_info, &process_info_)) {
    SSF_LOG(kLogError) << "session[process]: create process <" << binary_path_
                       << "> failed";
    ec.assign(::error::process_not_created, ::error::get_ssf_category());
  }

  ::CloseHandle(proc_out_);
  ::CloseHandle(proc_err_);
  ::CloseHandle(proc_in_);

  proc_out_ = INVALID_HANDLE_VALUE;
  proc_err_ = INVALID_HANDLE_VALUE;
  proc_in_ = INVALID_HANDLE_VALUE;
}

template <typename Demux>
void Session<Demux>::InitPipes(boost::system::error_code& ec) {
  auto local_fib_ep = client_.remote_endpoint(ec);
  if (ec) {
    SSF_LOG(kLogError)
        << "session[process]: could not get fiber local endpoint";
    return;
  }

  std::string remote_port(std::to_string(local_fib_ep.port()));
  out_pipe_name_ += remote_port;
  err_pipe_name_ += remote_port;
  in_pipe_name_ += remote_port;

  DWORD pipe_size = 4096;
  SECURITY_ATTRIBUTES sec_attr;
  sec_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
  sec_attr.bInheritHandle = TRUE;
  sec_attr.lpSecurityDescriptor = NULL;

  InitOutNamedPipe(out_pipe_name_, &data_out_, &proc_out_, &sec_attr, pipe_size,
                   ec);
  if (ec) {
    SSF_LOG(kLogError) << "session[process]: init out pipe failed";
    return;
  }

  InitOutNamedPipe(err_pipe_name_, &data_err_, &proc_err_, &sec_attr, pipe_size,
                   ec);
  if (ec) {
    SSF_LOG(kLogError) << "session[process]: init err pipe failed";
    return;
  }

  InitInNamedPipe(in_pipe_name_, &proc_in_, &data_in_, &sec_attr, pipe_size,
                  ec);
  if (ec) {
    SSF_LOG(kLogError) << "session[process]: init in pipe failed";
    return;
  }
}

template <typename Demux>
void Session<Demux>::InitOutNamedPipe(const std::string& pipe_name,
                                      HANDLE* p_read_pipe, HANDLE* p_write_pipe,
                                      SECURITY_ATTRIBUTES* p_pipe_attributes,
                                      DWORD pipe_size,
                                      boost::system::error_code& ec) {
  HANDLE read_pipe_tmp = ::CreateNamedPipeA(
      pipe_name.c_str(), PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, pipe_size, pipe_size,
      0, p_pipe_attributes);

  if (read_pipe_tmp == INVALID_HANDLE_VALUE) {
    SSF_LOG(kLogError) << "session[process]: create read side of named pipe <"
                       << pipe_name << "> failed";
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    goto cleanup;
  }

  *p_write_pipe =
      ::CreateFileA(pipe_name.c_str(), FILE_WRITE_DATA | SYNCHRONIZE, 0,
                    p_pipe_attributes, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

  if (*p_write_pipe == INVALID_HANDLE_VALUE) {
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    SSF_LOG(kLogError) << "session[process]: create write side of named pipe <"
                       << pipe_name << "> failed";
    goto cleanup;
  }

  if (!::DuplicateHandle(GetCurrentProcess(), read_pipe_tmp,
                         GetCurrentProcess(), p_read_pipe, 0, FALSE,
                         DUPLICATE_SAME_ACCESS)) {
    SSF_LOG(kLogError)
        << "session[process]: duplicate read side of named pipe <" << pipe_name
        << "> failed";
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    goto cleanup;
  }

cleanup:
  if (read_pipe_tmp != INVALID_HANDLE_VALUE) {
    ::CloseHandle(read_pipe_tmp);
  }
}

template <typename Demux>
void Session<Demux>::InitInNamedPipe(const std::string& pipe_name,
                                     HANDLE* p_read_pipe, HANDLE* p_write_pipe,
                                     SECURITY_ATTRIBUTES* p_pipe_attributes,
                                     DWORD pipe_size,
                                     boost::system::error_code& ec) {
  HANDLE write_pipe_tmp = ::CreateNamedPipeA(
      pipe_name.c_str(), PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, pipe_size, pipe_size,
      0, p_pipe_attributes);

  if (write_pipe_tmp == INVALID_HANDLE_VALUE) {
    SSF_LOG(kLogError) << "session[process]: create write side of named pipe <"
                       << pipe_name << "> failed";
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    goto cleanup;
  }

  *p_read_pipe =
      ::CreateFileA(pipe_name.c_str(), FILE_READ_DATA | SYNCHRONIZE, 0,
                    p_pipe_attributes, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

  if (*p_read_pipe == INVALID_HANDLE_VALUE) {
    SSF_LOG(kLogError) << "session[process]: create read side of named pipe <"
                       << pipe_name << "> failed";
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    goto cleanup;
  }

  if (!::DuplicateHandle(::GetCurrentProcess(), write_pipe_tmp,
                         ::GetCurrentProcess(), p_write_pipe, 0, FALSE,
                         DUPLICATE_SAME_ACCESS)) {
    SSF_LOG(kLogError)
        << "session[process]: duplicate write side of named pipe <" << pipe_name
        << "> failed";
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    goto cleanup;
  }

cleanup:
  if (write_pipe_tmp != INVALID_HANDLE_VALUE) {
    ::CloseHandle(write_pipe_tmp);
  }
}

}  // windows
}  // process
}  // services
}  // ssf

#endif  // SSF_SERVICES_PROCESS_WINDOWS_SESSION_IPP_