#ifndef SSF_SERVICES_SOCKETS_TO_FIBERS_SOCKETS_TO_FIBERS_IPP_
#define SSF_SERVICES_SOCKETS_TO_FIBERS_SOCKETS_TO_FIBERS_IPP_

#include <ssf/log/log.h>
#include <ssf/network/session_forwarder.h>

namespace ssf {
namespace services {
namespace sockets_to_fibers {

template <typename Demux>
SocketsToFibers<Demux>::SocketsToFibers(boost::asio::io_service& io_service,
                                        demux& fiber_demux, uint16_t local_port,
                                        remote_port_type remote_port)
    : ssf::BaseService<Demux>::BaseService(io_service, fiber_demux),
      local_port_(local_port),
      remote_port_(remote_port),
      socket_acceptor_(io_service),
      socket_(io_service),
      fiber_(io_service) {}

template <typename Demux>
void SocketsToFibers<Demux>::start(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo)
      << "service[sockets to fibers]: starting relay on local port tcp "
      << local_port_;

  boost::system::error_code close_ec;
  // Accept on all local interfaces
  boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(),
                                          local_port_);

  socket_acceptor_.open(endpoint.protocol(), ec);
  if (ec) {
    socket_acceptor_.close(close_ec);
    return;
  }

  boost::asio::socket_base::reuse_address option(true);

  socket_acceptor_.bind(endpoint, ec);
  if (ec) {
    socket_acceptor_.close(close_ec);
    return;
  }

  socket_acceptor_.set_option(option, ec);
  if (ec) {
    socket_acceptor_.close(close_ec);
    return;
  }

  socket_acceptor_.listen(boost::asio::socket_base::max_connections, ec);
  if (ec) {
    socket_acceptor_.close(close_ec);
    return;
  }

  this->StartAcceptSockets();
}

template <typename Demux>
void SocketsToFibers<Demux>::stop(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "service[sockets to fibers]: stopping";
  socket_acceptor_.close(ec);
  if (ec) {
    SSF_LOG(kLogDebug) << "service[sockets to fibers]: " << ec.message()
                       << std::endl;
  }
  manager_.stop_all();
}

template <typename Demux>
uint32_t SocketsToFibers<Demux>::service_type_id() {
  return factory_id;
}

template <typename Demux>
void SocketsToFibers<Demux>::StartAcceptSockets() {
  SSF_LOG(kLogTrace) << "service[sockets to fibers]: accepting new clients";

  if (!socket_acceptor_.is_open()) {
    return;
  }

  socket_acceptor_.async_accept(
      socket_,
      Then(&SocketsToFibers::SocketAcceptHandler, this->SelfFromThis()));
}

template <typename Demux>
void SocketsToFibers<Demux>::SocketAcceptHandler(
    const boost::system::error_code& ec) {
  SSF_LOG(kLogTrace) << "service[sockets to fibers]: accept handler";

  if (!socket_acceptor_.is_open()) {
    return;
  }

  if (!ec) {
    endpoint ep(this->get_demux(), remote_port_);
    fiber_.async_connect(
        ep, Then(&SocketsToFibers::FiberConnectHandler, this->SelfFromThis()));
  }
}

template <typename Demux>
void SocketsToFibers<Demux>::FiberConnectHandler(
    const boost::system::error_code& ec) {
  SSF_LOG(kLogTrace) << "service[sockets to fibers]: connect handler";

  if (!ec) {
    auto session = SessionForwarder<socket, fiber>::create(
        &manager_, std::move(socket_), std::move(fiber_));
    boost::system::error_code e;
    manager_.start(session, e);
  }

  StartAcceptSockets();
}

}  // sockets_to_fibers
}  // services
}  // ssf

#endif  // SSF_SERVICES_SOCKETS_TO_FIBERS_SOCKETS_TO_FIBERS_IPP_
