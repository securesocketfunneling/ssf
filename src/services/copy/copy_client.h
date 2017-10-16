#ifndef SSF_SERVICES_COPY_COPY_CLIENT_H_
#define SSF_SERVICES_COPY_COPY_CLIENT_H_

#ifdef WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include <cstdint>

#include <list>
#include <memory>
#include <string>

#include <boost/asio/write.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "common/filesystem/filesystem.h"
#include "common/filesystem/path.h"

#include "core/client/client.h"

#include "services/copy/config.h"
#include "services/copy/copy_context.h"
#include "services/copy/packet/control.h"
#include "services/copy/packet_helper.h"

#include "services/copy/copy_server.h"
#include "services/copy/file_acceptor.h"
#include "services/copy/file_sender.h"

namespace ssf {
namespace services {
namespace copy {

class CopyClient;
using CopyClientPtr = std::shared_ptr<CopyClient>;

class CopyClient : public std::enable_shared_from_this<CopyClient> {
 public:
  using Session = ssf::Client::ClientSession;
  using SessionPtr = std::shared_ptr<Session>;

  using OnFileStatus = std::function<void(CopyContext* context,
                                          const boost::system::error_code& ec)>;
  using OnFileCopied = std::function<void(CopyContext* context,
                                          const boost::system::error_code& ec)>;
  using OnCopyFinished =
      std::function<void(uint64_t files_count, uint64_t error_count,
                         const boost::system::error_code& ec)>;

 private:
  using Demux = Session::Demux;
  using SocketType = typename Demux::socket_type;
  using Fiber = typename boost::asio::fiber::stream_fiber<SocketType>::socket;
  using FiberPtr = std::shared_ptr<Fiber>;
  using Endpoint =
      typename boost::asio::fiber::stream_fiber<SocketType>::endpoint;

  using CopyServer = ssf::services::copy::CopyServer<Demux>;
  using FileAcceptor = FileAcceptor<Demux>;
  using FileAcceptorPtr = FileAcceptor::Ptr;
  using FileSender = FileSender<Demux>;
  using FileSenderPtr = typename FileSender::Ptr;

  using OnFiberConnect =
      std::function<void(const boost::system::error_code& ec)>;

 public:
  static CopyClientPtr Create(SessionPtr session, OnFileStatus on_file_status,
                              OnFileCopied on_file_copied,
                              OnCopyFinished on_copy_finished,
                              boost::system::error_code& ec) {
    if (!session) {
      SSF_LOG(kLogError) << "microservice[copy][client] invalid session";
      ec.assign(::error::bad_file_descriptor, ::error::get_ssf_category());
      return nullptr;
    }

    CopyClientPtr client(new CopyClient(session, on_file_status, on_file_copied,
                                        on_copy_finished));

    client->Init(ec);
    if (ec) {
      return nullptr;
    }

    return client;
  }

  ~CopyClient() {
    SSF_LOG(kLogDebug) << "microservice[copy][client] destroy";
    if (copy_request_.is_from_stdin) {
// reset stdin in text mode
#ifdef WIN32
      if (_setmode(_fileno(stdin), _O_TEXT) == -1) {
      }
#endif
    }
  }

  void AsyncCopyToServer(const CopyRequest& req) {
    auto self = this->shared_from_this();
    auto on_connect = [this, self, req](const boost::system::error_code& ec) {
      if (ec) {
        on_copy_finished_(0, 0, ec);
        SSF_LOG(kLogError)
            << "microservice[copy][client] could not connect control channel";
        return;
      }
      CopyToServer(req);
    };
    ConnectControlChannel(on_connect);
  }

  void AsyncCopyFromServer(const CopyRequest& req) {
    auto self = this->shared_from_this();
    auto on_connect = [this, self, req](const boost::system::error_code& ec) {
      if (ec) {
        on_copy_finished_(0, 0, ec);
        SSF_LOG(kLogError)
            << "microservice[copy][client] could not connect control channel";
        return;
      }
      CopyFromServer(req);
    };
    ConnectControlChannel(on_connect);
  }

 private:
  CopyClient(SessionPtr session, OnFileStatus on_file_status,
             OnFileCopied on_file_copied, OnCopyFinished on_copy_finished)
      : session_(session),
        control_fiber_(session->get_io_service()),
        file_acceptor_(FileAcceptor::Create(session->get_io_service())),
        on_file_status_(on_file_status),
        on_file_copied_(on_file_copied),
        on_copy_finished_(on_copy_finished) {}

  void Init(boost::system::error_code& ec) {
    if (copy_request_.is_from_stdin) {
// set stdin in binary mode
#ifdef WIN32
      if (_setmode(_fileno(stdin), _O_BINARY) == -1) {
        SSF_LOG(kLogError)
            << "microservice[copy][client] cannot set binary mode on stdin";
        ec.assign(::error::bad_file_descriptor, ::error::get_ssf_category());
        return;
      }
#endif
    }
  }

  void ConnectControlChannel(OnFiberConnect on_fiber_connect) {
    Endpoint ep(session_->GetDemux(), CopyServer::kControlServicePort);
    control_fiber_.async_connect(ep, on_fiber_connect);
  }

  void CopyToServer(const CopyRequest& req) {
    SSF_LOG(kLogDebug) << "microservice[copy][client] copy data to server";

    boost::system::error_code ec;
    file_sender_ = FileSender::Create(
        session_->GetDemux(), std::move(control_fiber_), req, on_file_status_,
        on_file_copied_, on_copy_finished_, ec);
    if (ec) {
      SSF_LOG(kLogError)
          << "microservice[copy][client] cannot create file sender";
      NotifyCopyFinished(0, 0, ErrorCode(ec.value()));
      return;
    }
    file_sender_->AsyncSend();
  }

  void CopyFromServer(const CopyRequest& req) {
    SSF_LOG(kLogDebug) << "microservice[copy][client] copy data from server";

    boost::system::error_code ec;
    auto self = this->shared_from_this();
    auto packet = std::make_shared<Packet>();

    file_acceptor_->Listen(session_->GetDemux(), ec);
    if (ec) {
      SSF_LOG(kLogDebug)
          << "microservice[copy][client] cannot accept new files";
      NotifyCopyFinished(0, 0, ErrorCode(ec.value()));
      return;
    }

    file_acceptor_->AsyncAccept(on_file_status_, on_file_copied_);

    auto on_copy_request_ack =
        [this, self, packet](const boost::system::error_code& ec) {
          if (ec) {
            SSF_LOG(kLogError)
                << "microservice[copy][client] could not receive copy reply";
            NotifyCopyFinished(0, 0, ErrorCode::kCopyRequestAckNotReceived);
            return;
          }
          boost::system::error_code convert_ec;
          CopyRequestAck req_ack;
          PacketToPayload(*packet, req_ack, convert_ec);
          if (convert_ec ||
              req_ack.status != CopyRequestAck::kRequestReceived) {
            SSF_LOG(kLogError)
                << "microservice[copy][client] copy request not received";
            NotifyCopyFinished(0, 0, ErrorCode::kCopyRequestCorrupted);
            return;
          }

          file_acceptor_->AsyncAccept(on_file_status_, on_file_copied_);
          AsyncReadControlRequest();
        };

    auto on_copy_request_sent =
        [this, self, packet,
         on_copy_request_ack](const boost::system::error_code& ec) {
          if (ec) {
            NotifyCopyFinished(0, 0, ErrorCode::kClientCopyRequestNotSent);
            SSF_LOG(kLogError)
                << "microservice[copy][client] could not write copy request";
            return;
          }
          AsyncReadPacket(control_fiber_, *packet, on_copy_request_ack);
        };
    AsyncWritePayload(control_fiber_, req, packet, on_copy_request_sent);
  }

  void AsyncReadControlRequest() {
    SSF_LOG(kLogDebug) << "microservice[copy][client] async read request";
    auto self = this->shared_from_this();
    auto packet = std::make_shared<Packet>();
    auto on_packet_read = [this, self,
                           packet](const boost::system::error_code& ec) {
      if (ec) {
        SSF_LOG(kLogDebug) << "microservice[copy][client] "
                              "error while reading packet";
        return;
      }

      // process current request
      OnRequest(packet);
    };
    AsyncReadPacket(control_fiber_, *packet, on_packet_read);
  }

  void OnRequest(PacketPtr packet) {
    switch (packet->type()) {
      case PacketType::kCopyFinished:
        OnCopyFinishedNotification(packet);
      default:
        // noop
        break;
    };
    AsyncReadControlRequest();
  }

  void OnCopyFinishedNotification(PacketPtr packet) {
    boost::system::error_code convert_ec;
    CopyFinishedNotification notification;
    PacketToPayload(*packet, notification, convert_ec);
    if (convert_ec) {
      SSF_LOG(kLogDebug) << "microservice[copy][client] could not convert "
                            "packet to CopyFinishedNotification";
      NotifyCopyFinished(0, 0, ErrorCode::kUnknown);
    } else {
      NotifyCopyFinished(notification.files_count, notification.errors_count,
                         notification.error_code);
    }
  }

  void NotifyCopyFinished(uint64_t files_count, uint64_t errors_count,
                          ErrorCode error_code) {
    auto self = this->shared_from_this();
    boost::system::error_code copy_finished_ec;
    copy_finished_ec.assign(error_code, get_copy_category());
    session_->get_io_service().post(
        [this, self, files_count, errors_count, copy_finished_ec]() {
          on_copy_finished_(files_count, errors_count, copy_finished_ec);
        });
  }

 private:
  SessionPtr session_;
  Fiber control_fiber_;
  FileAcceptorPtr file_acceptor_;
  FileSenderPtr file_sender_;
  CopyRequest copy_request_;
  ssf::Filesystem fs_;
  OnFileStatus on_file_status_;
  OnFileCopied on_file_copied_;
  OnCopyFinished on_copy_finished_;
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_COPY_CLIENT_H_
