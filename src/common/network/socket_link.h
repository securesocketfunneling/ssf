#ifndef SSF_COMMON_NETWORK_SOCKET_LINK_H_
#define SSF_COMMON_NETWORK_SOCKET_LINK_H_

#include <boost/tuple/tuple.hpp>  // NOLINT

#include <boost/asio/coroutine.hpp>
#include <boost/asio/buffer.hpp>  // NOLINT
#include <boost/system/error_code.hpp>   // NOLINT

namespace ssf {
//-----------------------------------------------------------------------------

/// Async Half Duplex Stream Socket Forwarder
/**
* @tparam Handler type of the callback handler
* @tparam ReadFromSocketType type of the input socket
* @tparam WriteToSocketType type od the output socket
*/
template<class Handler,
         class ReadFromSocketType,
         class WriteToSocketType = ReadFromSocketType>
struct AsyncHDSocketLinker : boost::asio::coroutine {
 public:
  /// Constructor
  /**
  * @param read_from input socket
  * @param write_to output socket
  * @param working_buffer a single buffer to receive and send
  * @param handler the callback to call when the transfer stops
  */
  AsyncHDSocketLinker(ReadFromSocketType& read_from,
                      WriteToSocketType& write_to,
                      boost::asio::mutable_buffers_1 working_buffer,
                      Handler handler)
      : r_(read_from),
        w_(write_to),
        working_buffer_(working_buffer),
        handler_(handler),
        transfered_bytes_(0) { }

#include <boost/asio/detail/win_iocp_io_service.hpp>
#include <boost/asio/yield.hpp>  // NOLINT

  /// Operator()
  /**
  * This is function is its own handler for asynchronous operations it calls
  *
  * @param ec an error code describing the status of the operation
  * @param n number of bytes handled
  */
  void operator() (const boost::system::error_code& ec, std::size_t n) {
    if (!ec && r_.is_open() && w_.is_open()) reenter(this) {
      for (;;) {
        // Receive some data
        yield r_.async_read_some(working_buffer_, std::move(*this));
        transfered_bytes_ = n;

        // Keep sending until the number of sent bytes is not null (if some
        // bytes were received previously).
        do {
          // Send the received data
          yield boost::asio::async_write(
              w_, boost::asio::buffer(working_buffer_, transfered_bytes_),
              std::move(*this));
        } while (!n && transfered_bytes_);
      }
    }
    else {
      boost::get<0>(handler_)(ec);
    }
  }
#include <boost/asio/unyield.hpp>  // NOLINT

  ReadFromSocketType& r_;
  WriteToSocketType& w_;
  boost::asio::mutable_buffers_1 working_buffer_;
  Handler handler_;
  size_t transfered_bytes_;
};

/// Wrapper for the Stream Socket to read from
template<class SocketType>
struct ReadFromHelper {
  typedef SocketType type;

  explicit ReadFromHelper(SocketType& ref)
    : read_from_(ref) { }

  SocketType& read_from_;
};

/// Function to create a ReadFromHelper
template<typename SocketType>
ReadFromHelper<SocketType> ReadFrom(SocketType& s) {
  return ReadFromHelper<SocketType>(s);
}

/// Wrapper for the Stream Socket to write to
template<class SocketType>
struct WriteToHelper {
  typedef SocketType type;

  explicit WriteToHelper(SocketType& ref)
    : write_to_(ref) { }

  SocketType& write_to_;
};

/// Function to create a WriteToHelper
template<typename SocketType>
WriteToHelper<SocketType> WriteTo(SocketType& s) {
  return WriteToHelper<SocketType>(s);
}

/// Establish a Half Duplex Link
template<typename Handler, class ReadFrom, class WriteTo>
void AsyncEstablishHDLink(ReadFrom rf, WriteTo wt,
                          boost::asio::mutable_buffers_1 working_buffer,
                          Handler handler) {
  AsyncHDSocketLinker<boost::tuple<Handler>,
                      typename ReadFrom::type,
                      typename WriteTo::type> AsyncTransfer(
        rf.read_from_,
        wt.write_to_,
        working_buffer,
        boost::make_tuple(handler));

  AsyncTransfer(boost::system::error_code(), 0);
}

}  // ssf

#endif  // SSF_COMMON_NETWORK_SOCKET_LINK_H_
