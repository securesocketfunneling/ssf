#ifndef SSF_SERVICES_COPY_FILE_SENDER_H_
#define SSF_SERVICES_COPY_FILE_SENDER_H_

#include <list>
#include <memory>

#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "common/filesystem/filesystem.h"
#include "common/filesystem/path.h"

#include "services/copy/copy_context.h"
#include "services/copy/copy_session.h"
#include "services/copy/error_code.h"
#include "services/copy/packet/control.h"
#include "services/copy/packet_helper.h"
#include "services/copy/state/sender/send_init_request_state.h"

#include "services/copy/file_acceptor.h"

namespace ssf {
namespace services {
namespace copy {

template <class Demux>
class FileSender : public std::enable_shared_from_this<FileSender<Demux>> {
 public:
  using Ptr = std::shared_ptr<FileSender>;

  using OnFileStatus = std::function<void(CopyContext* context,
                                          const boost::system::error_code& ec)>;
  using OnFileCopied = std::function<void(CopyContext* context,
                                          const boost::system::error_code& ec)>;
  using OnCopyFinished =
      std::function<void(uint64_t files_count, uint64_t errors_count,
                         const boost::system::error_code& ec)>;

 private:
  using SocketType = typename Demux::socket_type;
  using Fiber = typename boost::asio::fiber::stream_fiber<SocketType>::socket;
  using FiberPtr = std::shared_ptr<Fiber>;
  using Endpoint =
      typename boost::asio::fiber::stream_fiber<SocketType>::endpoint;
  using SessionManager = ItemManager<BaseSessionPtr>;

  using OnControlConnected = std::function<void(
      FiberPtr p_fiber, const boost::system::error_code& ec)>;

 public:
  static Ptr Create(Demux& demux, Fiber control_fiber, const CopyRequest& req,
                    OnFileStatus on_file_status, OnFileCopied on_file_copied,
                    OnCopyFinished on_copy_finished,
                    boost::system::error_code& ec) {
    return Ptr(new FileSender(demux, std::move(control_fiber), req,
                              on_file_status, on_file_copied,
                              on_copy_finished));
  }

  ~FileSender() {
    SSF_LOG("microservice", trace, "[copy][file_sender] destroy");
  }

  void AsyncSend() {
    if (copy_request_.is_from_stdin) {
      AsyncSendStdin();
    } else {
      AsyncSendFiles();
    }
  }

  void AsyncSendFiles() {
    SSF_LOG("microservice", debug, "[copy][file_sender] async send files");
    RunFileSessions();
  }

  void AsyncSendStdin() {
    SSF_LOG("microservice", debug, "[copy][file_sender] async send stdin");
    RunStdinSession();
  }

  void Stop() {
    SSF_LOG("microservice", debug, "[copy][file_sender] stop");
    stopped_ = true;

    // stop all current sessions
    manager_.stop_all();

    // notify all pending files
    boost::system::error_code copy_ec(ErrorCode::kCopyStopped,
                                      get_copy_category());
    while (!pending_input_files_.empty()) {
      ssf::Path input_file;
      {
        std::lock_guard<std::recursive_mutex> lock(input_files_mutex_);
        if (pending_input_files_.empty() ||
            input_files_.size() > copy_request_.max_parallel_copies) {
          // noop if pending_input_files is empty or max parallel copies reached
          return;
        }
        input_file = pending_input_files_.front();
        input_files_.emplace_back(input_file);
        pending_input_files_.pop_front();
      }

      boost::system::error_code context_ec;
      auto context = GenerateFileContext(input_file, context_ec);
      NotifyFileCopied(context.get(), copy_ec);
    }
    worker_.reset();
  }

  uint64_t input_files_count() { return input_files_count_; }

 private:
  FileSender(Demux& demux, Fiber control_fiber, const CopyRequest& req,
             OnFileStatus on_file_status, OnFileCopied on_file_copied,
             OnCopyFinished on_copy_finished)
      : io_service_(demux.get_io_service()),
        demux_(demux),
        worker_(std::make_unique<boost::asio::io_service::work>(
            demux.get_io_service())),
        control_fiber_(std::move(control_fiber)),
        copy_request_(req),
        input_files_count_(0),
        copy_errors_count_(0),
        copy_ec_(ErrorCode::kSuccess),
        stopped_(false),
        on_file_status_(on_file_status),
        on_file_copied_(on_file_copied),
        on_copy_finished_(on_copy_finished) {}

  void RunFileSessions() {
    boost::system::error_code ec;
    ListInputFiles(ec);
    if (ec) {
      SSF_LOG("microservice", debug,
              "[copy][file_sender] cannot list input files");
      NotifyCopyFinished(ErrorCode(ec.value()));
    }

    if (pending_input_files_.empty()) {
      NotifyCopyFinished(ErrorCode::kSuccess);
      return;
    }

    RunFileSession();
  }

  void RunFileSession() {
    ssf::Path input_file;
    {
      std::lock_guard<std::recursive_mutex> lock(input_files_mutex_);
      if (stopped_ || pending_input_files_.empty() ||
          input_files_.size() >= copy_request_.max_parallel_copies) {
        // noop if stopped or pending_input_files is empty or max parallel
        // copies reached
        return;
      }
      input_file = pending_input_files_.front();
      input_files_.emplace_back(input_file);
      pending_input_files_.pop_front();
    }

    SSF_LOG("microservice", debug, "[copy][file_sender] start copy {}",
            input_file.GetString());

    auto self = this->shared_from_this();

    FiberPtr fiber = std::make_shared<Fiber>(io_service_);

    auto on_file_connected = [this, self, input_file,
                              fiber](const boost::system::error_code& ec) {
      boost::system::error_code session_ec;
      auto context = GenerateFileContext(input_file, session_ec);
      if (session_ec) {
        SSF_LOG("microservice", debug,
                "[copy][file_sender] could not "
                "generate copy context for {} ({})",
                input_file.GetString(), ec.message());
        NotifyFileCopied(context.get(), ec);
        return;
      }

      if (ec) {
        boost::system::error_code copy_ec(ErrorCode::kNetworkError,
                                          get_copy_category());
        SSF_LOG("microservice", debug,
                "[copy][file_sender] could not connect file fiber {}",
                ec.message());
        NotifyFileCopied(context.get(), copy_ec);
        return;
      }

      StartSession(fiber, std::move(context), session_ec);
      if (session_ec) {
        SSF_LOG("microservice", debug,
                "[copy][file_sender] could not "
                "start copy session for {} ({})",
                input_file.GetString(), ec.message());
        return;
      }
    };

    if (stopped_) {
      boost::system::error_code network_ec(ErrorCode::kCopyStopped,
                                           get_copy_category());
      on_file_connected(network_ec);
      return;
    }

    if (!stopped_ && !pending_input_files_.empty()) {
      io_service_.post([this, self]() { RunFileSession(); });
    }

    Endpoint ep(demux_, FileAcceptor<Demux>::kPort);

    SSF_LOG("microservice", debug,
            "[copy][file_sender] connect to file acceptor port {}",
            FileAcceptor<Demux>::kPort);
    fiber->async_connect(ep, on_file_connected);
  }

  void RunStdinSession() {
    auto self = this->shared_from_this();
    FiberPtr fiber = std::make_shared<Fiber>(io_service_);

    SSF_LOG("microservice", debug,
            "[copy][file_sender] connect to file acceptor port {}",
            FileAcceptor<Demux>::kPort);

    auto on_file_connected = [this, self,
                              fiber](const boost::system::error_code& ec) {
      boost::system::error_code session_ec;
      auto context = GenerateStdinContext(session_ec);
      if (session_ec) {
        SSF_LOG("microservice", debug,
                "[copy][file_sender] could not "
                "generate stdin copy context ({})",
                ec.message());
        return;
      }
      StartSession(fiber, std::move(context), session_ec);
      if (session_ec) {
        SSF_LOG("microservice", debug,
                "[copy][file_sender] could not "
                "start stdin copy session ({})",
                ec.message());
        return;
      }
    };

    if (stopped_) {
      boost::system::error_code network_ec(ErrorCode::kInterrupted,
                                           get_copy_category());
      io_service_.post(
          [on_file_connected, network_ec]() { on_file_connected(network_ec); });
      return;
    }

    Endpoint ep(demux_, FileAcceptor<Demux>::kPort);
    fiber->async_connect(ep, on_file_connected);
  }

  void ConnectControlChannel(OnControlConnected on_control_connected) {}

  CopyContextUPtr GenerateFileContext(const Path& input_filepath,
                                      boost::system::error_code& ec) {
    CopyContextUPtr context = std::make_unique<CopyContext>(io_service_);

    Path input_dir(copy_request_.input_pattern);
    if (!fs_.IsDirectory(input_dir, ec)) {
      input_dir = input_dir.GetParent();
    }

    Path output_path(copy_request_.output_pattern);
    Path output_directory = output_path;
    Path output_filename = input_filepath;
    if (fs_.IsFile(copy_request_.input_pattern, ec)) {
      // output_pattern should be a file path instead of a directory path
      output_directory = output_path.GetParent();
      output_filename = output_path.GetFilename();
    }
    ec.clear();

    ICopyStateUPtr send_init_request_state = SendInitRequestState::Create();
    context->SetState(std::move(send_init_request_state));
    boost::system::error_code fs_ec;
    auto filesize = context->fs.GetFilesize(input_dir / input_filepath, fs_ec);
    if (fs_ec) {
      filesize = 0;
    }

    context->Init(
        input_dir.GetString(), input_filepath.GetString(),
        copy_request_.check_file_integrity,
        copy_request_.is_from_stdin, 0, copy_request_.is_resume, filesize,
        output_directory.GetString(), output_filename.GetString());

    return context;
  }

  CopyContextUPtr GenerateStdinContext(boost::system::error_code& ec) {
    Path output_path(copy_request_.output_pattern);
    Path output_directory = output_path.GetParent();
    Path output_filename = output_path.GetFilename();

    CopyContextUPtr context = std::make_unique<CopyContext>(io_service_);

    ICopyStateUPtr send_init_request_state = SendInitRequestState::Create();
    context->SetState(std::move(send_init_request_state));

    context->Init("", "", false, copy_request_.is_from_stdin, 0,
                  copy_request_.is_resume, 0, output_directory.GetString(),
                  output_filename.GetString());

    return context;
  }

  void StartSession(FiberPtr p_fiber, CopyContextUPtr context,
                    const boost::system::error_code& connect_ec) {
    auto self = this->shared_from_this();

    auto on_file_status = [this, self](CopyContext* context,
                                       const boost::system::error_code& ec) {
      NotifyFileStatus(context, ec);
    };

    auto on_file_copied = [this, self](BaseSessionPtr session,
                                       CopyContext* context,
                                       const boost::system::error_code& ec) {
      NotifyFileCopied(context, ec);
      boost::system::error_code stop_ec;
      manager_.stop(session, stop_ec);
    };

    if (context->is_stdin_input) {
      SSF_LOG("microservice", debug, "[copy][file_sender] send stdin to {}",
              context->GetOutputFilepath().GetString());
    } else {
      SSF_LOG("microservice", debug,
              "[copy][file_sender] send file {} to {}",
              context->GetInputFilepath().GetString(),
              context->GetOutputFilepath().GetString());
    }

    boost::system::error_code start_ec;
    auto p_session =
        CopySession<Fiber>::Create(std::move(*p_fiber), std::move(context),
                                   on_file_status, on_file_copied);
    manager_.start(p_session, start_ec);
  }

  void ListInputFiles(boost::system::error_code& ec) {
    std::lock_guard<std::recursive_mutex> lock(input_files_mutex_);
    boost::system::error_code fs_ec;
    SSF_LOG("microservice", trace, "[copy][file_sender] list input files");
    std::list<Path> files;
    bool is_file = fs_.IsFile(copy_request_.input_pattern, fs_ec);
    if (is_file) {
      pending_input_files_.emplace_back(Path(copy_request_.input_pattern).GetFilename());
    } else {
      fs_ec.clear();
      pending_input_files_ = fs_.ListFiles(copy_request_.input_pattern,
                                           copy_request_.is_recursive, fs_ec);
      if (fs_ec) {
        SSF_LOG("microservice", debug,
                "[copy][file_sender] could not list input files");
        ec.assign(ErrorCode::kSenderInputFileListingFailed,
                  get_copy_category());
        return;
      }
    }
    input_files_count_ = pending_input_files_.size();
  }

  void NotifyFileStatus(CopyContext* context,
                        const boost::system::error_code& ec) {
    if (!stopped_) {
      on_file_status_(context, ec);
    }
  }

  void NotifyFileCopied(CopyContext* context,
                        const boost::system::error_code& ec) {
    auto is_copy_finished = true;
    if (context->is_stdin_input) {
      SSF_LOG("microservice", debug, "[copy][file_sender] stdin copied {}",
              ec.message());
    } else {
      SSF_LOG("microservice", debug, "[copy][file_sender] file {} copied {}",
              context->GetInputFilepath().GetString(), ec.message());
      std::lock_guard<std::recursive_mutex> lock(input_files_mutex_);
      input_files_.remove(context->input_filename);

      if (ec) {
        ++copy_errors_count_;
      }
      is_copy_finished = input_files_.empty() && pending_input_files_.empty();
    }

    if (!stopped_) {
      on_file_copied_(context, ec);
      // run next copy
      if (!context->is_stdin_input) {
        RunFileSession();
      }
    }

    if (!is_copy_finished) {
      return;
    }

    // copy finished
    ErrorCode copy_finished_ec = ErrorCode::kSuccess;
    if (input_files_count_ == 1 || copy_request_.is_from_stdin) {
      copy_finished_ec = ErrorCode(ec.value());
    } else if (copy_errors_count_ == input_files_count_) {
      copy_finished_ec = ErrorCode::kNoFileCopied;
    } else if (copy_errors_count_ > 0) {
      copy_finished_ec = ErrorCode::kFilesPartiallyCopied;
    }
    NotifyCopyFinished(copy_finished_ec);
  }

  void NotifyCopyFinished(ErrorCode error_code) {
    auto self = this->shared_from_this();

    auto on_copy_finished = on_copy_finished_;
    on_copy_finished_ = [](uint64_t files_count, uint64_t error_count,
                           const boost::system::error_code& ec) {};

    auto packet = std::make_shared<Packet>();
    uint64_t files_count = input_files_count_;
    uint64_t errors_count = copy_errors_count_;

    CopyFinishedNotification finished_notification(files_count, errors_count,
                                                   error_code);
    AsyncWritePayload(
        control_fiber_, finished_notification, packet,
        [this, self, packet](const boost::system::error_code& ec) {
          SSF_LOG("microservice", debug,
                  "[copy][file_sender] send copy finished notification");
        });

    boost::system::error_code copy_finished_ec(error_code, get_copy_category());
    on_copy_finished(files_count, errors_count, copy_finished_ec);
  }

 private:
  boost::asio::io_service& io_service_;
  Demux& demux_;
  std::unique_ptr<boost::asio::io_service::work> worker_;
  Fiber control_fiber_;
  CopyRequest copy_request_;

  SessionManager manager_;
  std::recursive_mutex input_files_mutex_;
  std::list<Path> pending_input_files_;
  std::list<Path> input_files_;
  uint64_t input_files_count_;
  ssf::Filesystem fs_;
  uint64_t copy_errors_count_;
  ErrorCode copy_ec_;
  bool stopped_;

  OnFileStatus on_file_status_;
  OnFileCopied on_file_copied_;
  OnCopyFinished on_copy_finished_;
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_FILE_SENDER_H_
