#ifndef SSF_SERVICES_COPY_FILE_ACCEPTOR_H_
#define SSF_SERVICES_COPY_FILE_ACCEPTOR_H_

#include <memory>

#include <boost/asio/io_service.hpp>

#include "common/boost/fiber/stream_fiber.hpp"

#include "services/copy/copy_context.h"
#include "services/copy/copy_session.h"
#include "services/copy/packet/control.h"
#include "services/copy/packet_helper.h"
#include "services/copy/state/receiver/wait_init_request_state.h"
#include "services/service_port.h"

namespace ssf {
namespace services {
namespace copy {

template <class Demux>
class FileAcceptor : public std::enable_shared_from_this<FileAcceptor<Demux>> {
 public:
  using Ptr = std::shared_ptr<FileAcceptor>;

  using OnAsyncAccept = std::function<void(boost::system::error_code& ec)>;
  using OnFileStatus = std::function<void(CopyContext* context,
                                          const boost::system::error_code& ec)>;
  using OnFileCopied = std::function<void(CopyContext* context,
                                          const boost::system::error_code& ec)>;

  enum { kPort = ServicePort::kCopyFileAcceptor };

 private:
  using SocketType = typename Demux::socket_type;
  using Fiber = typename boost::asio::fiber::stream_fiber<SocketType>::socket;
  using FiberPtr = std::shared_ptr<Fiber>;
  using FiberAcceptor =
      typename boost::asio::fiber::stream_fiber<SocketType>::acceptor;
  using Endpoint =
      typename boost::asio::fiber::stream_fiber<SocketType>::endpoint;

  using SessionManager = ItemManager<BaseSessionPtr>;

 public:
  static Ptr Create(boost::asio::io_service& io_service) {
    return Ptr(new FileAcceptor(io_service));
  }

  ~FileAcceptor() {
    SSF_LOG(kLogDebug) << "microservice[copy][file_acceptor] destroy";
  }

  void Listen(Demux& demux, boost::system::error_code& ec) {
    // init file acceptor
    Endpoint file_ep(demux, kPort);
    fiber_acceptor_.bind(file_ep, ec);
    if (ec) {
      ec.assign(ErrorCode::kFileAcceptorNotBound, get_copy_category());
      SSF_LOG(kLogDebug)
          << "microservice[copy][file_acceptor] cannot bind acceptor";
      return;
    }
    SSF_LOG(kLogDebug) << "microservice[copy][file_acceptor]start accepting "
                          "file transfer on fiber port "
                       << kPort;
    fiber_acceptor_.listen(boost::asio::socket_base::max_connections, ec);
    if (ec) {
      ec.assign(ErrorCode::kFileAcceptorNotListening, get_copy_category());
      return;
    }
  }

  void AsyncAccept(OnFileStatus on_file_status, OnFileCopied on_file_copied) {
    if (!fiber_acceptor_.is_open()) {
      return;
    }

    auto self = this->shared_from_this();
    auto p_fiber = std::make_shared<Fiber>(fiber_acceptor_.get_io_service());

    auto on_file_fiber_accept =
        [this, self, p_fiber, on_file_status,
         on_file_copied](const boost::system::error_code& ec) {
          if (ec) {
            SSF_LOG(kLogDebug) << "microservice[copy][file_acceptor] could not "
                                  "accept new file transfer";
            return;
          }

          AsyncAccept(on_file_status, on_file_copied);

          ReceiveFile(p_fiber, on_file_status, on_file_copied);
        };
    fiber_acceptor_.async_accept(*p_fiber, std::move(on_file_fiber_accept));
  }

  void Close(boost::system::error_code& close_ec) {
    manager_.stop_all();
    fiber_acceptor_.close(close_ec);
  }

 private:
  FileAcceptor(boost::asio::io_service& io_service)
      : fiber_acceptor_(io_service),
        worker_(std::make_unique<boost::asio::io_service::work>(io_service)) {}

  void ReceiveFile(FiberPtr p_fiber, OnFileStatus on_file_status,
                   OnFileCopied on_file_copied) {
    auto self = this->shared_from_this();

    ICopyStateUPtr wait_request_state = WaitInitRequestState::Create();
    CopyContextUPtr context = std::make_unique<CopyContext>(
        p_fiber->get_io_service(), std::move(wait_request_state));

    auto on_session_file_status =
        [this, self, on_file_status](CopyContext* context,
                                     const boost::system::error_code& ec) {
          SSF_LOG(kLogInfo)
              << "microservice[copy][file_acceptor] receiver on file status "
              << ec.message();
          on_file_status(context, ec);
        };

    auto on_session_file_copied =
        [this, self, on_file_copied](CopyContext* context,
                                     const boost::system::error_code& ec) {
          SSF_LOG(kLogDebug)
              << "microservice[copy][file_acceptor] receiver file copied "
              << context->input_filepath << " " << ec.message();
          on_file_copied(context, ec);
        };

    boost::system::error_code start_ec;
    auto p_session = CopySession<Fiber>::Create(
        &manager_, std::move(*p_fiber), std::move(context),
        on_session_file_status, on_session_file_copied);
    manager_.start(p_session, start_ec);
  }

 private:
  FiberAcceptor fiber_acceptor_;
  std::unique_ptr<boost::asio::io_service::work> worker_;
  OnFileCopied on_file_copied_;
  SessionManager manager_;
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_FILE_ACCEPTOR_H_
