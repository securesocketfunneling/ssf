#ifndef SSF_SERVICES_SOCKETS_TO_FIBERS_SOCKETS_TO_FIBERS_IPP_
#define SSF_SERVICES_SOCKETS_TO_FIBERS_SOCKETS_TO_FIBERS_IPP_

#include <ssf/log/log.h>
#include "services/sockets_to_fibers/session.h"

namespace ssf {
namespace services {
namespace sockets_to_fibers {

template <typename Demux>
SocketsToFibers<Demux>::SocketsToFibers(boost::asio::io_service& io_service,
                                        Demux& fiber_demux,
                                        const std::string& local_addr,
                                        LocalPortType local_port,
                                        RemotePortType remote_port)
    : ssf::BaseService<Demux>::BaseService(io_service, fiber_demux),
      local_addr_(local_addr),
      local_port_(local_port),
      remote_port_(remote_port),
      socket_acceptor_(io_service) {}

template <typename Demux>
void SocketsToFibers<Demux>::start(boost::system::error_code& ec) {
  Tcp::resolver resolver(this->get_io_service());
  Tcp::resolver::query query(local_addr_, std::to_string(local_port_));
  auto ep_it = resolver.resolve(query, ec);
  if (ec) {
    SSF_LOG(kLogError) << "microservice[stream_listener]: could not resolve "
                          "query <" << local_addr_ << ":" << local_port_ << ">";
    return;
  }

  Tcp::endpoint endpoint(*ep_it);

  boost::system::error_code close_ec;
  socket_acceptor_.open(endpoint.protocol(), ec);
  if (ec) {
    SSF_LOG(kLogError)
        << "microservice[stream_listener]: could not open acceptor";
    socket_acceptor_.close(close_ec);
    return;
  }

  boost::asio::socket_base::reuse_address option(true);
  socket_acceptor_.set_option(option, ec);
  if (ec) {
    SSF_LOG(kLogError) << "microservice[stream_listener]: could not set "
                          "reuse address option";
    socket_acceptor_.close(close_ec);
    return;
  }

  socket_acceptor_.bind(endpoint, ec);
  if (ec) {
    SSF_LOG(kLogError)
        << "microservice[stream_listener]: could not bind acceptor to <"
        << local_addr_ << ":" << local_port_ << ">";
    socket_acceptor_.close(close_ec);
    return;
  }

  socket_acceptor_.listen(boost::asio::socket_base::max_connections, ec);
  if (ec) {
    SSF_LOG(kLogError)
        << "microservice[stream_listener]: could not listen new connections";
    socket_acceptor_.close(close_ec);
    return;
  }

  SSF_LOG(kLogInfo)
      << "microservice[stream_listener]: forward TCP connections from <"
      << local_addr_ << ":" << local_port_ << "> to fiber port "
      << remote_port_;

  this->AsyncAcceptSocket();
}

template <typename Demux>
void SocketsToFibers<Demux>::stop(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "microservice[stream_listener]: stopping";
  socket_acceptor_.close(ec);
  if (ec) {
    SSF_LOG(kLogDebug) << "microservice[stream_listener]: " << ec.message();
  }
  manager_.stop_all();
}

template <typename Demux>
uint32_t SocketsToFibers<Demux>::service_type_id() {
  return kFactoryId;
}

template <typename Demux>
void SocketsToFibers<Demux>::StopSession(BaseSessionPtr session,
                                         boost::system::error_code& ec) {
  manager_.stop(session, ec);
}

template <typename Demux>
void SocketsToFibers<Demux>::AsyncAcceptSocket() {
  SSF_LOG(kLogTrace) << "microservice[stream_listener]: accepting new clients";

  if (!socket_acceptor_.is_open()) {
    return;
  }

  std::shared_ptr<Tcp::socket> socket_connection =
      std::make_shared<Tcp::socket>(this->get_io_service());

  socket_acceptor_.async_accept(
      *socket_connection,
      boost::bind(&SocketsToFibers::SocketAcceptHandler, this->SelfFromThis(),
                  socket_connection, _1));
}

template <typename Demux>
void SocketsToFibers<Demux>::SocketAcceptHandler(
    std::shared_ptr<Tcp::socket> socket_connection,
    const boost::system::error_code& ec) {
  SSF_LOG(kLogTrace) << "microservice[stream_listener]: accept handler";

  if (ec) {
    SSF_LOG(kLogDebug)
        << "microservice[stream_listener]: error accepting new connection: "
        << ec.message() << " " << ec.value();
    return;
  }

  if (socket_acceptor_.is_open()) {
    this->AsyncAcceptSocket();
  }

  FiberPtr fiber_connection = std::make_shared<Fiber>(this->get_io_service());
  FiberEndpoint ep(this->get_demux(), remote_port_);
  fiber_connection->async_connect(
      ep,
      boost::bind(&SocketsToFibers::FiberConnectHandler, this->SelfFromThis(),
                  fiber_connection, socket_connection, _1));
}

template <typename Demux>
void SocketsToFibers<Demux>::FiberConnectHandler(
    FiberPtr fiber_connection, std::shared_ptr<Tcp::socket> socket_connection,
    const boost::system::error_code& ec) {
  SSF_LOG(kLogTrace) << "microservice[stream_listener]: connect handler";

  if (ec) {
    SSF_LOG(kLogError)
        << "microservice[stream_listener]: error connecting to remote fiber";
    boost::system::error_code close_ec;
    socket_connection->close(close_ec);
    return;
  }

  auto session = Session<Demux, Tcp::socket, Fiber>::create(
      this->SelfFromThis(), std::move(*socket_connection),
      std::move(*fiber_connection));
  boost::system::error_code start_ec;
  manager_.start(session, start_ec);
  if (start_ec) {
    SSF_LOG(kLogError) << "microservice[stream_listener]: cannot start session";
    start_ec.clear();
    session->stop(start_ec);
  }
}

}  // sockets_to_fibers
}  // services
}  // ssf

#endif  // SSF_SERVICES_SOCKETS_TO_FIBERS_SOCKETS_TO_FIBERS_IPP_
