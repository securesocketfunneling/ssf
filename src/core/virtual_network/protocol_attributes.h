#ifndef SSF_CORE_VIRTUAL_NETWORK_PROTOCOL_ATTRIBUTES_H_
#define SSF_CORE_VIRTUAL_NETWORK_PROTOCOL_ATTRIBUTES_H_

#include <type_traits>

#include <boost/bind.hpp>

#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

namespace virtual_network {

enum overhead { multiplexed = 1 << 1, header = 1 << 2, footer = 1 << 3 };
enum facilities {
  raw = 1 << 1,
  promiscuous = 1 << 2,
  datagram = 1 << 3,
  stream = 1 << 4
};

template <class Socket>
struct IsStream {
  enum { value = !!(Socket::protocol_type::facilities & facilities::stream) };
};

template <class Socket>
struct IsDatagram {
  enum { value = !!(Socket::protocol_type::facilities & facilities::datagram) };
};

//-----------------------------------------------------------------//

template <class Socket, class Datagram, class Endpoint, class Handler>
void AsyncSendDatagram(
    Socket& socket, const Datagram& datagram, const Endpoint& destination,
    Handler handler,
    typename std::enable_if<IsStream<Socket>::value, Socket>::type* = nullptr) {
  boost::asio::async_write(socket, datagram.GetConstBuffers(),
                           std::move(handler));
}

template <class Socket, class Datagram, class Handler>
void AsyncSendDatagram(
    Socket& socket, const Datagram& datagram, Handler handler,
    typename std::enable_if<IsStream<Socket>::value, Socket>::type* = nullptr) {
  boost::asio::async_write(socket, datagram.GetConstBuffers(),
                           std::move(handler));
}

template <class Socket, class Datagram, class Endpoint, class Handler>
void AsyncReceiveDatagram(
    Socket& socket, Datagram* p_datagram, Endpoint& source,
    Handler handler,
    typename std::enable_if<IsStream<Socket>::value, Socket>::type* = nullptr) {
  source = socket.remote_endpoint();
  AsyncReceiveDatagram(socket, p_datagram, std::move(handler));
}

template <class Socket, class Datagram, class Handler>
void AsyncReceiveDatagram(
    Socket& socket, Datagram* p_datagram,
    const Handler& handler,
    typename std::enable_if<IsStream<Socket>::value, Socket>::type* = nullptr) {
  auto footer_received_lambda = [p_datagram, handler](
      std::size_t total_length, const boost::system::error_code& ec,
      std::size_t length) { handler(ec, total_length + length); };

  auto payload_received_lambda = [&socket, p_datagram, footer_received_lambda](
    std::size_t total_length, const boost::system::error_code& ec,
    std::size_t length) {
    if (!ec) {
      boost::asio::async_read(
        socket, p_datagram->footer().GetMutableBuffers(),
        std::move(boost::bind<void>(std::move(footer_received_lambda),
        total_length + length, _1, _2)));
    } else {
      footer_received_lambda(total_length + length, ec, 0);
    }
  };

  auto header_received_lambda = [&socket, p_datagram, payload_received_lambda](
    const boost::system::error_code& ec, std::size_t length) {
    if (!ec) {
      p_datagram->payload().SetSize(p_datagram->header().payload_length());
      boost::asio::async_read(
        socket, p_datagram->payload().GetMutableBuffers(),
        std::move(boost::bind<void>(std::move(payload_received_lambda),
        length, _1, _2)));
    } else {
      payload_received_lambda(length, ec, 0);
    }
  };

  boost::asio::async_read(socket, p_datagram->header().GetMutableBuffers(),
    std::move(header_received_lambda));
}

template <class Socket, class Datagram, class Endpoint, class Handler>
void AsyncSendDatagram(
    Socket& socket, const Datagram& datagram, const Endpoint& destination,
    Handler handler,
    typename std::enable_if<IsDatagram<Socket>::value, Socket>::type* = nullptr) {
  socket.async_send_to(datagram.GetConstBuffers(), destination,
                       std::move(handler));
}

template <class Socket, class Datagram, class Handler>
void AsyncSendDatagram(
  Socket& socket, const Datagram& datagram,
  Handler handler,
  typename std::enable_if<IsDatagram<Socket>::value, Socket>::type* = nullptr) {
  socket.async_send(datagram.GetConstBuffers(), std::move(handler));
}

template <class Socket, class Datagram, class Endpoint, class Handler>
void AsyncReceiveDatagram(
    Socket& socket, Datagram* p_datagram, Endpoint& source,
    const Handler& handler,
    typename std::enable_if<IsDatagram<Socket>::value, Socket>::type* = nullptr) {
  auto p_buffer = std::make_shared<std::vector<uint8_t>>(
      Datagram::size + Socket::protocol_type::mtu);

  auto datagram_received_lambda = [p_buffer, p_datagram, handler](
      const boost::system::error_code& ec, std::size_t length) {
    p_datagram->payload().SetSize(length - Datagram::size);
    boost::asio::buffer_copy(p_datagram->GetMutableBuffers(),
                             boost::asio::buffer(*p_buffer));
    handler(ec, length);
  };

  socket.async_receive_from(boost::asio::buffer(*p_buffer), source,
                            std::move(datagram_received_lambda));
}

template <class Socket, class Datagram, class Handler>
void AsyncReceiveDatagram(
    Socket& socket, Datagram* p_datagram,
    const Handler& handler,
    typename std::enable_if<IsDatagram<Socket>::value, Socket>::type* = nullptr) {
  auto p_buffer = std::make_shared<std::vector<uint8_t>>(
    Datagram::size + Socket::protocol_type::mtu);

  auto datagram_received_lambda = [p_buffer, p_datagram, handler](
      const boost::system::error_code& ec, std::size_t length) {
    p_datagram->payload().SetSize(length - Datagram::size);
    boost::asio::buffer_copy(p_datagram->GetMutableBuffers(),
      boost::asio::buffer(*p_buffer));
    handler(ec, length);
  };

  socket.async_receive(boost::asio::buffer(*p_buffer),
                       std::move(datagram_received_lambda));
}

}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_PROTOCOL_ATTRIBUTES_H_
