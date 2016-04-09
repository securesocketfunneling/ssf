#ifndef SRC_SERVICES_COPY_FILE_FILE_TO_FIBER_ISTREAM_TO_FIBER_SESSION_H_
#define SRC_SERVICES_COPY_FILE_FILE_TO_FIBER_ISTREAM_TO_FIBER_SESSION_H_

#include <memory>
#include <set>
#include <string>

#include <boost/asio/coroutine.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

#include <boost/system/error_code.hpp>

#include <ssf/network/base_session.h>
#include <ssf/network/manager.h>
#include <ssf/network/base_session.h>

#include "services/copy_file/filename_buffer.h"
#include "services/copy_file/packet/packet.h"

namespace ssf {
namespace services {
namespace copy_file {
namespace file_to_fiber {

template <class InputStream, class OutputSocketStream>
class IstreamToFiberSession : public ssf::BaseSession {
 public:
  using IstreamToFiberSessionPtr = std::shared_ptr<IstreamToFiberSession>;
  // Type for the class managing the different forwarding links
  using SessionManager = ItemManager<BaseSessionPtr>;
  // Buffer for the transiting data
  using FilenameBuffer = ssf::services::copy_file::FilenameBuffer;
  using Packet = ssf::services::copy_file::packet::Packet;
  using StopSessionHandler = std::function<void(const std::string&)>;

 public:
  template <typename... Args>
  static IstreamToFiberSessionPtr Create(Args&&... args) {
    return std::shared_ptr<IstreamToFiberSession>(
        new IstreamToFiberSession(std::forward<Args>(args)...));
  }

  virtual ~IstreamToFiberSession() {}

  // Start read from input stream and write to output socket stream
  virtual void start(boost::system::error_code& ec) {
    if (!from_stdin_) {
      input_stream_.open(input_file_, InputStream::binary);
      if (!input_stream_.is_open() || !input_stream_.good()) {
        BOOST_LOG_TRIVIAL(error) << "session istream to fiber : cannot open file "
                                 << input_file_;
        ec.assign(::error::bad_file_descriptor, ::error::get_ssf_category());
        stop_handler_(output_file_);

        return;
      }
    }

    ForwardData(boost::system::error_code(), 0);
  }

  // Stop session
  virtual void stop(boost::system::error_code& ec) {
    output_socket_stream_.close(ec);
    input_stream_.close();
  }

 private:
  IstreamToFiberSession(SessionManager* p_manager, const std::string& input_file,
                        OutputSocketStream&& output_socket_stream,
                        const std::string& output_file,
                        StopSessionHandler stop_handler)
      : from_stdin_(false),
        input_file_(input_file),
        input_stream_(),
        output_socket_stream_(std::move(output_socket_stream)),
        output_file_(output_file),
        p_manager_(p_manager),
        stop_handler_(stop_handler),
        output_request_(output_file) {}

  IstreamToFiberSession(SessionManager* p_manager,
                        OutputSocketStream&& output_socket_stream,
                        const std::string& output_file,
                        StopSessionHandler stop_handler)
      : from_stdin_(true),
        output_socket_stream_(std::move(output_socket_stream)),
        output_file_(output_file),
        p_manager_(p_manager),
        stop_handler_(stop_handler),
        output_request_(output_file) {}

#include <boost/asio/yield.hpp>  // NOLINT
  // Write filename
  // Forward file through fiber
  void ForwardData(const boost::system::error_code& ec, std::size_t length) {
    auto& input = from_stdin_ ? std::cin : input_stream_;

    if (ec) {
      boost::system::error_code stop_ec;
      p_manager_->stop(this->SelfFromThis(), stop_ec);
      stop_handler_(output_file_);

      return;
    }

    reenter(coroutine_) {
      // async send output file name size
      yield boost::asio::async_write(
          output_socket_stream_, output_request_.GetFilenameSizeConstBuffers(),
          boost::bind(&IstreamToFiberSession::ForwardData, this->SelfFromThis(),
                      _1, _2));

      // async send output file name
      yield boost::asio::async_write(
          output_socket_stream_, output_request_.GetFilenameConstBuffers(),
          boost::bind(&IstreamToFiberSession::ForwardData, this->SelfFromThis(),
                      _1, _2));

      while (input.good()) {
        packet_.set_type(Packet::kData);
        input.read(packet_.buffer().data(), packet_.buffer().size());
        packet_.set_size(static_cast<uint32_t>(input.gcount()));

        yield boost::asio::async_write(
            output_socket_stream_, packet_.GetConstBuf(),
            boost::bind(&IstreamToFiberSession::ForwardData,
                        this->SelfFromThis(), _1, _2));
      }

      if (input.eof()) {
        packet_.set_type(Packet::kCtrl);
        packet_.set_signal(Packet::kEof);
      } else {
        packet_.set_type(Packet::kCtrl);
        packet_.set_signal(Packet::kInterrupted);
      }

      yield boost::asio::async_write(
          output_socket_stream_, packet_.GetConstBuf(),
          boost::bind(&IstreamToFiberSession::ForwardData, this->SelfFromThis(),
                      _1, _2));

      yield boost::asio::async_read(
          output_socket_stream_, packet_.GetTypeMutBuf(),
          boost::bind(&IstreamToFiberSession::ForwardData, this->SelfFromThis(),
                      _1, _2));
    }
  }
#include <boost/asio/unyield.hpp>  // NOLINT

  IstreamToFiberSessionPtr SelfFromThis() {
    return std::static_pointer_cast<IstreamToFiberSession>(
        this->shared_from_this());
  }

 private:
  bool from_stdin_;
  std::string input_file_;
  InputStream input_stream_;
  OutputSocketStream output_socket_stream_;
  std::string output_file_;
  SessionManager* p_manager_;
  StopSessionHandler stop_handler_;

  // Coroutine variables
  boost::asio::coroutine coroutine_;
  Packet packet_;
  std::size_t file_read_length_;
  FilenameBuffer output_request_;
};

}  // file_to_fiber
}  // copy_file
}  // services
}  // ssf

#endif  // SRC_SERVICES_COPY_FILE_FILE_TO_FIBER_ISTREAM_TO_FIBER_SESSION_H_
