#ifndef SSF_SERVICES_PROCESS_WINDOWS_SESSION_IPP_
#define SSF_SERVICES_PROCESS_WINDOWS_SESSION_IPP_

#include <sstream>

#include <shlobj.h>

#include <ssf/log/log.h>
#include <ssf/network/session_forwarder.h>
#include <ssf/network/socket_link.h>

#include "common/error/error.h"

namespace ssf {
namespace services {
namespace process {
namespace windows {

template <typename Demux>
Session<Demux>::Session(std::weak_ptr<ShellServer> server, Fiber client,
                        const std::string& binary_path,
                        const std::string& binary_args)
    : ssf::BaseSession(),
      io_service_(client.get_io_service()),
      p_server_(server),
      client_(std::move(client)),
      binary_path_(binary_path),
      binary_args_(binary_args),
      out_pipe_name_("\\\\.\\pipe\\out_pipe_"),
      err_pipe_name_("\\\\.\\pipe\\err_pipe_"),
      in_pipe_name_("\\\\.\\pipe\\in_pipe_"),
      data_in_(INVALID_HANDLE_VALUE),
      data_out_(INVALID_HANDLE_VALUE),
      data_err_(INVALID_HANDLE_VALUE),
      proc_in_(INVALID_HANDLE_VALUE),
      proc_out_(INVALID_HANDLE_VALUE),
      proc_err_(INVALID_HANDLE_VALUE),
      job_(INVALID_HANDLE_VALUE),
      h_out_(client_.get_io_service()),
      h_err_(client_.get_io_service()),
      h_in_(client_.get_io_service()) {
  memset(&process_info_, 0, sizeof(PROCESS_INFORMATION));
  process_info_.hProcess = INVALID_HANDLE_VALUE;
  process_info_.hThread = INVALID_HANDLE_VALUE;
}

template <typename Demux>
void Session<Demux>::start(boost::system::error_code& ec) {
  SSF_LOG("microservice", debug, "[shell] session start");

  InitPipes(ec);
  if (ec) {
    SSF_LOG("microservice", error,
            "[shell] session pipes initialization failed");
    stop(ec);
    return;
  }

  StartProcess(ec);
  if (ec) {
    SSF_LOG("microservice", error, "[shell] session start process failed");
    stop(ec);
    return;
  }

  StartForwarding(ec);
  if (ec) {
    SSF_LOG("microservice", error,
            "[shell] session forwarding data from process to client failed");
    stop(ec);
    return;
  }
}

template <typename Demux>
void Session<Demux>::stop(boost::system::error_code& ec) {
  SSF_LOG("microservice", debug, "[shell] session stop");

  if (job_ != INVALID_HANDLE_VALUE) {
    if (!::TerminateJobObject(job_, 0)) {
      SSF_LOG("microservice", warn, "[shell] session stop: cannot terminate job");
      if (process_info_.hProcess != INVALID_HANDLE_VALUE) {
        ::TerminateProcess(process_info_.hProcess, 0);
      }
    }
    ::CloseHandle(job_);
    job_ = INVALID_HANDLE_VALUE;
  } else if (process_info_.hProcess != INVALID_HANDLE_VALUE) {
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
    SSF_LOG("microservice", error, "[shell] session stop error {}",
            ec.message());
  }
}

template <typename Demux>
void Session<Demux>::StopHandler(const boost::system::error_code& ec) {
  boost::system::error_code stop_err;
  if (auto server = p_server_.lock()) {
    server->StopSession(this->SelfFromThis(), stop_err);
  }
}

template <typename Demux>
std::shared_ptr<Session<Demux>> Session<Demux>::SelfFromThis() {
  return std::static_pointer_cast<Session>(this->shared_from_this());
}

template <typename Demux>
void Session<Demux>::StartForwarding(boost::system::error_code& ec) {
  h_out_.assign(data_out_, ec);
  if (ec) {
    SSF_LOG("microservice", error,
            "[shell] session could not initialize out stream handle");
    return;
  }
  data_out_ = INVALID_HANDLE_VALUE;
  h_err_.assign(data_err_, ec);
  if (ec) {
    SSF_LOG("microservice", error,
            "[shell] session could not initialize err stream handle");
    return;
  }
  data_err_ = INVALID_HANDLE_VALUE;
  h_in_.assign(data_in_, ec);
  if (ec) {
    SSF_LOG("microservice", error,
            "[shell] session could not initialize in stream handle");
    return;
  }
  data_in_ = INVALID_HANDLE_VALUE;

  // pipe process stdout to socket output
  AsyncEstablishHDLink(ReadFrom(h_out_), WriteTo(client_),
                       boost::asio::buffer(downstream_out_),
                       std::bind(&Session::StopHandler, this->SelfFromThis(),
                                 std::placeholders::_1));
  // pipe process stderr to socket output
  AsyncEstablishHDLink(ReadFrom(h_err_), WriteTo(client_),
                       boost::asio::buffer(downstream_err_),
                       std::bind(&Session::StopHandler, this->SelfFromThis(),
                                 std::placeholders::_1));
  // pipe socket input to process stdin
  AsyncEstablishHDLink(ReadFrom(client_), WriteTo(h_in_),
                       boost::asio::buffer(upstream_),
                       std::bind(&Session::StopHandler, this->SelfFromThis(),
                                 std::placeholders::_1));
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

  // chdir to home dir at process creation
  char home_dir[MAX_PATH];
  bool home_dir_set = SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL,
                                                 SHGFP_TYPE_CURRENT, home_dir));

  std::string command_line = binary_path_ + " " + binary_args_;

  if (::CreateProcessA(NULL, const_cast<char*>(command_line.c_str()), NULL,
                        NULL, TRUE, CREATE_NEW_CONSOLE, NULL,
                        (home_dir_set ? home_dir : NULL), &startup_info,
                        &process_info_)) {
    // try to create job object to kill process tree when session is stopped
    job_ = ::CreateJobObjectA(NULL, NULL);
    if (job_ != NULL) {
      if (!::AssignProcessToJobObject(job_, process_info_.hProcess)) {
        SSF_LOG("microservice", error,
                "[shell] session cannot assign process to job");
        ::CloseHandle(job_);
        job_ = INVALID_HANDLE_VALUE;
      }
    } else {
      SSF_LOG("microservice", error, "[shell] session job cannot be initialized");
    }
  } else {
    SSF_LOG("microservice", error, "[shell] session create process <{}> failed",
            command_line);
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
  auto fib_remote_ep = client_.remote_endpoint(ec);
  if (ec) {
    SSF_LOG("microservice", error,
            "[shell] session could not get fiber remote endpoint");
    return;
  }
  auto fib_local_ep = client_.local_endpoint(ec);
  if (ec) {
    SSF_LOG("microservice", error,
            "[shell] session could not get fiber local endpoint");
    return;
  }

  std::string local_port(std::to_string(fib_remote_ep.port()));
  std::string remote_port(std::to_string(fib_remote_ep.port()));
  std::string pipe_id = local_port + "_" + remote_port;
  out_pipe_name_ += pipe_id;
  err_pipe_name_ += pipe_id;
  in_pipe_name_ += pipe_id;

  DWORD pipe_size = 4096;
  SECURITY_ATTRIBUTES sec_attr;
  sec_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
  sec_attr.bInheritHandle = TRUE;
  sec_attr.lpSecurityDescriptor = NULL;

  InitOutNamedPipe(out_pipe_name_, &data_out_, &proc_out_, &sec_attr, pipe_size,
                   ec);
  if (ec) {
    SSF_LOG("microservice", error, "[shell] session init out pipe failed");
    return;
  }

  InitOutNamedPipe(err_pipe_name_, &data_err_, &proc_err_, &sec_attr, pipe_size,
                   ec);
  if (ec) {
    SSF_LOG("microservice", error, "[shell] session init err pipe failed");
    return;
  }

  InitInNamedPipe(in_pipe_name_, &proc_in_, &data_in_, &sec_attr, pipe_size,
                  ec);
  if (ec) {
    SSF_LOG("microservice", error, "[shell] session init in pipe failed");
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
    SSF_LOG("microservice", error,
            "[shell] session create read side of named pipe <{}> failed",
            pipe_name);
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    goto cleanup;
  }

  *p_write_pipe = ::CreateFileA(
      pipe_name.c_str(), FILE_WRITE_DATA | SYNCHRONIZE, 0, p_pipe_attributes,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);

  if (*p_write_pipe == INVALID_HANDLE_VALUE) {
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    SSF_LOG("microservice", error,
            "[shell] session create write side of named pipe <{}> failed",
            pipe_name);
    goto cleanup;
  }

  if (!::DuplicateHandle(GetCurrentProcess(), read_pipe_tmp,
                         GetCurrentProcess(), p_read_pipe, 0, FALSE,
                         DUPLICATE_SAME_ACCESS)) {
    SSF_LOG("microservice", error,
            "[shell] session duplicate read side of named pipe <{}> failed",
            pipe_name);
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
    SSF_LOG("microservice", error,
            "[shell] session create write side of named pipe <{}> failed",
            pipe_name);
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    goto cleanup;
  }

  *p_read_pipe = ::CreateFileA(pipe_name.c_str(), FILE_READ_DATA | SYNCHRONIZE,
                               0, p_pipe_attributes, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 0);

  if (*p_read_pipe == INVALID_HANDLE_VALUE) {
    SSF_LOG("microservice", error,
            "[shell] session create read side of named pipe <{}> failed",
            pipe_name);
    ec.assign(::error::broken_pipe, ::error::get_ssf_category());
    goto cleanup;
  }

  if (!::DuplicateHandle(::GetCurrentProcess(), write_pipe_tmp,
                         ::GetCurrentProcess(), p_write_pipe, 0, FALSE,
                         DUPLICATE_SAME_ACCESS)) {
    SSF_LOG("microservice", error,
            "[shell] session duplicate write side of named pipe <{}> failed",
            pipe_name);
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