#ifndef SRC_SERVICES_COPY_FILE_FIBER_TO_FILE_FIBER_TO_OSTREAM_SESSION_H_
#define SRC_SERVICES_COPY_FILE_FIBER_TO_FILE_FIBER_TO_OSTREAM_SESSION_H_

#include <memory>

#include <boost/asio/coroutine.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>
#include <ssf/network/base_session.h>  // NOLINT
#include <ssf/network/manager.h>
#include <ssf/network/base_session.h>

#include "services/copy_file/filename_buffer.h"
#include "services/copy_file/packet/packet.h"

namespace ssf {
namespace services {
namespace copy_file {
namespace fiber_to_file {

template <class InputSocketStream, class OutputStream>
class FiberToOstreamSession : public ssf::BaseSession {
 public:
  using StopHandler = std::function<void()>;
  using FiberToOstreamSessionPtr = std::shared_ptr<FiberToOstreamSession>;

  // Type for the class managing the different forwarding links
  using SessionManager = ItemManager<BaseSessionPtr>;
  // Buffer for the transiting data
  using Packet = ssf::services::copy_file::packet::Packet;

 public:
  template <typename... Args>
  static FiberToOstreamSessionPtr Create(Args&&... args) {
    return std::shared_ptr<FiberToOstreamSession>(
        new FiberToOstreamSession(std::forward<Args>(args)...));
  }

  // Start read from stream socket and write to output stream
  virtual void start(boost::system::error_code&) {
    ForwardData(boost::system::error_code(), 0);
  }

  // Stop session
  virtual void stop(boost::system::error_code& ec) {
    SSF_LOG(kLogDebug) << "session fiber to file: stopped";
    input_socket_stream_.close(ec);
    // connection interrupted without prior notification (broken pipe)
    // delete output file
    if (output_stream_.is_open()) {
      output_stream_.close();
      std::remove(request_.GetFilename().c_str());
    }
  }

 private:
  FiberToOstreamSession(SessionManager* p_manager,
                        InputSocketStream input_socket_stream)
      : input_socket_stream_(std::move(input_socket_stream)),
        p_manager_(p_manager),
        output_stream_ok_(true) {}

#include <boost/asio/yield.hpp>  // NOLINT
  // Read output filename
  // Transfer data from fiber to file
  void ForwardData(const boost::system::error_code& ec, std::size_t length) {
    if (ec) {
      boost::system::error_code stop_ec;
      // connection interrupted without prior notification (broken pipe)
      if (output_stream_.is_open()) {
        output_stream_.close();
        std::remove(request_.GetFilename().c_str());
      }
      p_manager_->stop(this->SelfFromThis(), stop_ec);

      return;
    }

    reenter(coroutine_) {
      // read filename_size from request
      yield boost::asio::async_read(
          input_socket_stream_, request_.GetFilenameSizeMutBuffers(),
          boost::bind(&FiberToOstreamSession::ForwardData, this->SelfFromThis(),
                      _1, _2));

      // read filename from request
      yield boost::asio::async_read(
          input_socket_stream_, request_.GetFilenameMutBuffers(),
          boost::bind(&FiberToOstreamSession::ForwardData, this->SelfFromThis(),
                      _1, _2));

      SSF_LOG(kLogInfo)
          << "session fiber to file: start receiving data and writing in file "
          << request_.GetFilename();

      // open output_stream
      output_stream_.open(request_.GetFilename(), std::ofstream::binary);
      if (!output_stream_.is_open()) {
        SSF_LOG(kLogError) << "session fiber to file: output file "
                                 << request_.GetFilename()
                                 << " could not be opened";
        boost::system::error_code stop_ec;
        p_manager_->stop(this->SelfFromThis(), stop_ec);

        return;
      }

      while (input_socket_stream_.is_open()) {
        // Read type packet
        yield boost::asio::async_read(
            input_socket_stream_, packet_.GetTypeMutBuf(),
            boost::bind(&FiberToOstreamSession::ForwardData,
                        this->SelfFromThis(), _1, _2));

        if (packet_.IsDataPacket()) {
          // Read size packet
          yield boost::asio::async_read(
              input_socket_stream_, packet_.GetSizeMutBuf(),
              boost::bind(&FiberToOstreamSession::ForwardData,
                          this->SelfFromThis(), _1, _2));
          // Read binary data
          yield boost::asio::async_read(
              input_socket_stream_, packet_.GetPayloadMutBuf(),
              boost::bind(&FiberToOstreamSession::ForwardData,
                          this->SelfFromThis(), _1, _2));
          // Write in input file
          output_stream_.write(packet_.buffer().data(), packet_.size());
        } else {
          yield boost::asio::async_read(
              input_socket_stream_, packet_.GetSignalMutBuf(),
              boost::bind(&FiberToOstreamSession::ForwardData,
                          this->SelfFromThis(), _1, _2));

          // Close file handler
          output_stream_.close();
          if (packet_.signal() == Packet::kInterrupted) {
            // delete file -> transmission interrupted
            std::remove(request_.GetFilename().c_str());
          }
          break;
        }
      }

      // stop session
      boost::system::error_code stop_ec;
      p_manager_->stop(this->SelfFromThis(), stop_ec);
    }
  }
#include <boost/asio/unyield.hpp>  // NOLINT

  FiberToOstreamSessionPtr SelfFromThis() {
    return std::static_pointer_cast<FiberToOstreamSession>(
        this->shared_from_this());
  }

 private:
  InputSocketStream input_socket_stream_;
  SessionManager* p_manager_;

  // Coroutine variables
  boost::asio::coroutine coroutine_;
  bool output_stream_ok_;
  std::ofstream output_stream_;
  Packet packet_;
  FilenameBuffer request_;
};

}  // fiber_to_file
}  // copy_file
}  // services
}  // ssf

#endif  // SRC_SERVICES_COPY_FILE_FIBER_TO_FILE_FIBER_TO_OSTREAM_SESSION_H_
