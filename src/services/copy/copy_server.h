#ifndef SSF_SERVICES_COPY_COPY_SERVER_H_
#define SSF_SERVICES_COPY_COPY_SERVER_H_

#include <cstdint>

#include <list>
#include <memory>
#include <string>

#include <boost/asio/write.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "common/filesystem/filesystem.h"
#include "common/filesystem/path.h"
#include "common/utils/to_underlying.h"

#include "core/factories/service_factory.h"
#include "services/admin/requests/create_service_request.h"

#include "services/service_id.h"
#include "services/service_port.h"

#include "services/copy/config.h"
#include "services/copy/copy_context.h"
#include "services/copy/copy_session.h"
#include "services/copy/packet/control.h"
#include "services/copy/packet_helper.h"

#include "services/copy/file_acceptor.h"
#include "services/copy/file_sender.h"

namespace ssf {
namespace services {
namespace copy {

template <class Demux>
class CopyServer : public BaseService<Demux> {
 private:
  using CopyServerPtr = std::shared_ptr<CopyServer>;
  using Parameters = typename ssf::BaseService<Demux>::Parameters;

  using Fiber = typename ssf::BaseService<Demux>::fiber;
  using FiberPtr = std::shared_ptr<Fiber>;
  using Endpoint = typename ssf::BaseService<Demux>::endpoint;
  using FiberAcceptor = typename ssf::BaseService<Demux>::fiber_acceptor;

  using FileAcceptorPtr = typename FileAcceptor<Demux>::Ptr;
  using FileSenderPtr = typename FileSender<Demux>::Ptr;

 public:
  enum {
    kFactoryId = to_underlying(MicroserviceId::kCopyServer),
    kControlServicePort = to_underlying(MicroservicePort::kCopyServer)
  };

 public:
  // Factory method to create the service
  static CopyServerPtr Create(boost::asio::io_service& io_service,
                              Demux& fiber_demux,
                              const Parameters& parameters) {
    return CopyServerPtr(new CopyServer(io_service, fiber_demux));
  }

  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<Demux>> p_factory, const Config& config) {
    if (!config.enabled()) {
      // service factory is not enabled
      return;
    }

    auto creator = [](boost::asio::io_service& io_service, Demux& fiber_demux,
                      const Parameters& parameters) {
      return CopyServer::Create(io_service, fiber_demux, parameters);
    };
    p_factory->RegisterServiceCreator(kFactoryId, creator);
  }

  static ssf::services::admin::CreateServiceRequest<Demux> GetCreateRequest() {
    ssf::services::admin::CreateServiceRequest<Demux> request(kFactoryId);
    return request;
  }

 public:
  ~CopyServer() { SSF_LOG(kLogDebug) << "microservice[copy][server] destroy"; }

  // Start service
  void start(boost::system::error_code& ec) override {
    AcceptControlChannel(ec);
    if (ec) {
      return;
    }
    file_acceptor_->Listen(this->get_demux(), ec);
    if (ec) {
      return;
    }
    auto on_file_status = [](CopyContext* context,
                             const boost::system::error_code& ec) {
      if (context->is_stdin_input) {
        SSF_LOG(kLogDebug) << "microservice[copy][server] receive stdin > "
                           << context->GetOutputFilepath().GetString();
      } else {
        SSF_LOG(kLogDebug) << "microservice[copy][server] receive "
                           << context->input_filepath << " > "
                           << context->GetOutputFilepath().GetString();
      }
    };
    auto on_file_copied = [](CopyContext* context,
                             const boost::system::error_code& ec) {
      if (context->is_stdin_input) {
        SSF_LOG(kLogDebug) << "microservice[copy][server] copied stdin > "
                           << context->GetOutputFilepath().GetString();
      } else {
        SSF_LOG(kLogDebug) << "microservice[copy][server] copied "
                           << context->input_filepath << " > "
                           << context->GetOutputFilepath().GetString();
      }
    };
    file_acceptor_->AsyncAccept(on_file_status, on_file_copied);
  }

  // Stop service
  void stop(boost::system::error_code& ec) override {
    SSF_LOG(kLogDebug) << "microservice[copy][server] stop";
    boost::system::error_code close_ec;
    file_acceptor_->Close(close_ec);
    close_ec.clear();
    control_acceptor_.close(close_ec);
    if (file_sender_) {
      file_sender_->Stop();
    }
    file_sender_.reset();
  }

  uint32_t service_type_id() override { return kFactoryId; }

 private:
  CopyServer(boost::asio::io_service& io_service, Demux& fiber_demux)
      : BaseService<Demux>(io_service, fiber_demux),
        control_acceptor_(io_service),
        file_acceptor_(FileAcceptor<Demux>::Create(io_service)) {}

  void AcceptControlChannel(boost::system::error_code& ec) {
    // init control channel acceptor
    Endpoint control_ep(this->get_demux(), kControlServicePort);
    SSF_LOG(kLogDebug) << "microservice[copy][server] start accepting file "
                          "transfer on fiber port "
                       << kControlServicePort;
    control_acceptor_.bind(control_ep, ec);
    control_acceptor_.listen(boost::asio::socket_base::max_connections, ec);
    if (ec) {
      SSF_LOG(kLogError)
          << "microservice[copy][server] cannot accept control fiber";
      return;
    }
    AcceptControlFiber();
  }

  void AcceptControlFiber() {
    if (!control_acceptor_.is_open()) {
      return;
    }

    auto self = this->shared_from_this();
    auto control_fiber =
        std::make_shared<Fiber>(control_acceptor_.get_io_service());
    control_acceptor_.async_accept(
        *control_fiber,
        [this, self, control_fiber](const boost::system::error_code& ec) {
          if (ec) {
            SSF_LOG(kLogDebug)
                << "microservice[copy][server] could not accept control fiber";
            return;
          }

          // close control acceptor after first connection
          boost::system::error_code close_ec;
          // control_acceptor_.close(close_ec);

          AsyncReadControlRequest(control_fiber);
        });
  }

  void AsyncReadControlRequest(FiberPtr control_fiber) {
    SSF_LOG(kLogDebug) << "microservice[copy][server] async read request";
    auto self = this->shared_from_this();
    auto packet = std::make_shared<Packet>();
    AsyncReadPacket(*control_fiber, *packet,
                    [this, self, control_fiber,
                     packet](const boost::system::error_code& ec) {
                      if (ec) {
                        SSF_LOG(kLogDebug) << "microservice[copy][server] "
                                              "error while reading packet";
                        return;
                      }

                      // process current request
                      OnRequest(control_fiber, packet);
                    });
  }

  void OnRequest(FiberPtr control_fiber, PacketPtr packet) {
    switch (packet->type()) {
      case PacketType::kCopyRequest:
        OnCopyRequest(control_fiber, packet);
        return;
      default:
        // noop
        AsyncReadControlRequest(control_fiber);
        break;
    };
  }

  void OnCopyRequest(FiberPtr control_fiber, PacketPtr packet) {
    CopyRequest req;
    boost::system::error_code ec;
    PacketToPayload(*packet, req, ec);
    if (ec) {
      SSF_LOG(kLogDebug) << "microservice[copy][server] cannot convert packet "
                            "into CopyRequest";

      return;
    }

    CopyRequestAck ack(req, !ec ? CopyRequestAck::Status::kRequestReceived
                                : CopyRequestAck::Status::kRequestCorrupted);
    auto self = this->shared_from_this();

    AsyncWritePayload(
        *control_fiber, ack, packet,
        [this, self, control_fiber, req](const boost::system::error_code& ec) {
          if (ec) {
            SSF_LOG(kLogError) << "microservice[copy][server] cannot "
                                  "process copy request";
            boost::system::error_code close_ec;
            control_fiber->close(close_ec);
            return;
          }
          StartCopy(control_fiber, req);
        });
  }

  void StartCopy(FiberPtr control_fiber, const CopyRequest& req) {
    SSF_LOG(kLogDebug) << "microservice[copy][server] start copy";

    auto on_file_status = [](CopyContext* context,
                             const boost::system::error_code& ec) {
      SSF_LOG(kLogDebug) << "microservice[copy][server] send "
                         << context->input_filepath << " > "
                         << context->GetOutputFilepath().GetString();
    };
    auto on_file_copied = [](CopyContext* context,
                             const boost::system::error_code& ec) {
      SSF_LOG(kLogDebug) << "microservice[copy][server] send "
                         << context->input_filepath << " > "
                         << context->GetOutputFilepath().GetString() << " "
                         << ec.message();
    };
    auto self = this->shared_from_this();
    auto on_copy_finished = [this, self](uint64_t files_count,
                                         uint64_t errors_count,
                                         const boost::system::error_code& ec) {
      SSF_LOG(kLogDebug) << "microservice[copy][server] "
                         << (files_count - errors_count) << "/" << files_count
                         << " files copied";
    };

    boost::system::error_code ec;
    file_sender_ = FileSender<Demux>::Create(
        this->get_demux(), std::move(*control_fiber), req, on_file_status,
        on_file_copied, on_copy_finished, ec);
    if (ec) {
      SSF_LOG(kLogDebug) << "microservice[copy][server] cannot create file sender";
      return;
    }
    file_sender_->AsyncSend();
  }

 private:
  FiberAcceptor control_acceptor_;
  FileAcceptorPtr file_acceptor_;
  FileSenderPtr file_sender_;
  ssf::Filesystem fs_;
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_COPY_SERVER_H_
