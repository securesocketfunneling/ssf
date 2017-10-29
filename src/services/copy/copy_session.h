#ifndef SSF_SERVICES_COPY_COPY_SESSION_H_
#define SSF_SERVICES_COPY_COPY_SESSION_H_

#include <cstdint>

#include <functional>
#include <memory>

#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include <ssf/network/base_session.h>
#include <ssf/network/manager.h>

#include "services/copy/copy_context.h"
#include "services/copy/packet.h"
#include "services/copy/packet_helper.h"

namespace ssf {
namespace services {
namespace copy {

template <class SocketStream>
class CopySession : public ssf::BaseSession {
 public:
  using CopySessionPtr = std::shared_ptr<CopySession>;
  using SessionManager = ItemManager<BaseSessionPtr>;
  using OnFileStatus =
      std::function<void(CopyContext*, const boost::system::error_code&)>;
  using OnFileCopied = std::function<void(BaseSessionPtr, CopyContext*,
                                          const boost::system::error_code&)>;

 public:
  template <typename... Args>
  static CopySessionPtr Create(Args&&... args) {
    CopySessionPtr session(new CopySession(std::forward<Args>(args)...));
    auto on_state_changed = [session]() { session->OnStateChanged(); };
    session->context_->set_on_state_changed(on_state_changed);

    return session;
  }

  ~CopySession() {
    SSF_LOG(kLogDebug) << "microservice[copy][session] destroy";
  }

  void start(boost::system::error_code&) {
    AsyncWrite();
    AsyncRead();
  }

  void stop(boost::system::error_code& ec) {
    SSF_LOG(kLogDebug) << "microservice[copy][session] stop";
    socket_.close(ec);
    context_->Deinit();
    on_file_status_ = [](CopyContext*, const boost::system::error_code&) {};
    on_file_copied_ = [](BaseSessionPtr, CopyContext*,
                         const boost::system::error_code&) {};
  }

 private:
  CopySession(SocketStream socket, CopyContextUPtr context,
              OnFileStatus on_file_status, OnFileCopied on_file_copied)
      : socket_(std::move(socket)),
        context_(std::move(context)),
        on_file_status_(on_file_status),
        on_file_copied_(on_file_copied) {}

  void AsyncWrite() {
    boost::system::error_code fill_ec;

    auto self = this->shared_from_this();
    auto on_outbound_packet = [this, self](
        const boost::system::error_code& ec) { OnOutboundPacket(ec); };

    context_->AsyncFillOutboundPacket(&outbound_packet_, on_outbound_packet,
                                      fill_ec);
    if (fill_ec) {
      SSF_LOG(kLogDebug)
          << "microservice[copy][session] cannot fill outbound packet";
      StopSession();
      return;
    }
  }

  void OnOutboundPacket(const boost::system::error_code& ec) {
    if (ec) {
      SSF_LOG(kLogDebug)
          << "microservice[copy][session] fill outbound packet failed";
      StopSession();
      return;
    }

    if (outbound_packet_.type() == PacketType::kUnknown) {
      SSF_LOG(kLogDebug)
          << "microservice[copy][session] cannot send unknown packet type";
      StopSession();
      return;
    }

    auto self = this->shared_from_this();
    auto on_packet_sent = [this, self](const boost::system::error_code& ec,
                                       std::size_t length) {
      OnPacketSent(ec, length);
    };
    boost::asio::async_write(socket_, outbound_packet_.GetConstBuf(),
                             on_packet_sent);
  }

  void OnPacketSent(const boost::system::error_code& ec, std::size_t length) {
    if (ec) {
      SSF_LOG(kLogDebug)
          << "microservice[copy][session] could not send outbound packet";
      StopSession();
      return;
    }

    if (context_->filesize != 0 && context_->input.is_open()) {
      auto self = this->shared_from_this();
      on_file_status_(context_.get(), {});
    }

    if (context_->IsClosed()) {
      return;
    }

    AsyncWrite();
  }

  void AsyncRead() {
    auto self = this->shared_from_this();

    AsyncReadPacket(socket_, inbound_packet_,
                    [this, self](const boost::system::error_code& ec) {
                      OnPacketReceived(ec);
                    });
  }

  void OnPacketReceived(const boost::system::error_code& ec) {
    if (ec) {
      SSF_LOG(kLogDebug)
          << "microservice[copy][session] could not read packet payload";
      StopSession();
      return;
    }

    boost::system::error_code process_ec;
    context_->ProcessInboundPacket(inbound_packet_, process_ec);
    if (process_ec) {
      SSF_LOG(kLogDebug)
          << "microservice[copy][session] could not process inbound packet";
      StopSession();
      return;
    }

    if (context_->filesize != 0 && context_->output.is_open()) {
      auto self = this->shared_from_this();
      on_file_status_(context_.get(), {});
    }

    if (context_->IsClosed()) {
      return;
    }

    AsyncRead();
  }

  void OnStateChanged() {
    if (context_->IsClosed()) {
      StopSession();
      return;
    }

    boost::system::error_code ec;
    context_->FillOutboundPacket(ec);
  }

  void StopSession() {
    boost::system::error_code ec(ssf::services::copy::ErrorCode::kInterrupted,
                                 ssf::services::copy::get_copy_category());
    if (context_->IsTerminal()) {
      ec = context_->GetErrorCode();
    }
    on_file_copied_(this->shared_from_this(), context_.get(), ec);
  }

 private:
  SocketStream socket_;
  CopyContextUPtr context_;
  Packet inbound_packet_;
  Packet outbound_packet_;
  OnFileStatus on_file_status_;
  OnFileCopied on_file_copied_;
};

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_COPY_SESSION_H_
