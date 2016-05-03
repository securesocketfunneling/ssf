#ifndef SSF_NETWORK_OBJECT_IO_HELPERS_H_
#define SSF_NETWORK_OBJECT_IO_HELPERS_H_

#include <cstdint>

#include <memory>

#include <boost/system/error_code.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/buffer.hpp>

namespace ssf {

template <class Base, class Socket, class Handler>
void SendBase(Socket& socket, Base to_send, Handler handler) {
  auto p_to_send = std::make_shared<Base>(std::move(to_send));

  auto sent_lambda = [p_to_send, handler](
      const boost::system::error_code& ec, std::size_t length) { handler(ec); };

  boost::asio::async_write(
      socket, boost::asio::buffer(p_to_send.get(), sizeof(*p_to_send)),
      sent_lambda);
}

template <class Base, class Socket, class Handler>
void ReceiveBase(Socket& socket, Base* p_to_receive,
                 Handler handler) {
  auto received_lambda = [handler](
    const boost::system::error_code& ec, std::size_t length) mutable { handler(ec); };

  boost::asio::async_read(
      socket, boost::asio::buffer(p_to_receive, sizeof(*p_to_receive)),
      received_lambda);
}

template <class Socket, class Handler>
void SendString(Socket& socket, std::string to_send,
                Handler handler) {
  auto p_size = std::make_shared<uint32_t>((uint32_t)to_send.size());
  auto p_string = std::make_shared<std::string>(std::move(to_send));

  auto string_sent_lambda = [p_string, handler](
      const boost::system::error_code& ec, std::size_t length) mutable { handler(ec); };

  auto size_sent_lambda = [&socket, p_size, p_string, string_sent_lambda,
                           handler](const boost::system::error_code& ec,
                                    std::size_t length) mutable {
    if (!ec) {
      boost::asio::async_write(socket, boost::asio::buffer(*p_string),
                               string_sent_lambda);
    } else {
      handler(ec);
    }
  };

  boost::asio::async_write(socket,
                           boost::asio::buffer(p_size.get(), sizeof(*p_size)),
                           size_sent_lambda);
}

template <class Socket, class Handler>
void ReceiveString(Socket& socket, std::string* p_to_receive,
                   Handler handler) {
  auto p_size = std::make_shared<uint32_t>(0);

  auto string_received_lambda = [handler](
      const boost::system::error_code& ec, std::size_t length) mutable { handler(ec); };

  auto size_received_lambda = [&socket, p_size, p_to_receive,
                               string_received_lambda, handler](
      const boost::system::error_code& ec, std::size_t length) mutable {
    if (!ec) {
      if (*p_size > 1 * 1024 * 1024) {
        handler(boost::system::errc::make_error_code(
            boost::system::errc::argument_list_too_long));
        return;
      }

      p_to_receive->reserve(*p_size);
      p_to_receive->resize(*p_size);
      boost::asio::async_read(socket,
                              boost::asio::buffer(&(*p_to_receive)[0], *p_size),
                              string_received_lambda);
    } else {
      handler(ec);
    }
  };

  boost::asio::async_read(socket,
    boost::asio::buffer(p_size.get(), sizeof(*p_size)),
    size_received_lambda);
}

}  // ssf

#endif  // SSF_NETWORK_OBJECT_IO_HELPERS_H_
