#ifndef SSF_SERVICES_FIBERS_TO_SOCKETS_FIBERS_TO_SOCKETS_IPP_
#define SSF_SERVICES_FIBERS_TO_SOCKETS_FIBERS_TO_SOCKETS_IPP_

#include <ssf/log/log.h>
#include <ssf/network/session_forwarder.h>

#include "common/error/error.h"

namespace ssf {
namespace services {
namespace fibers_to_sockets {

template <typename Demux>
FibersToSockets<Demux>::FibersToSockets(boost::asio::io_service& io_service,
                                        Demux& fiber_demux,
                                        LocalPortType local_port,
                                        const std::string& ip,
                                        RemotePortType remote_port)
    : ssf::BaseService<Demux>::BaseService(io_service, fiber_demux),
      remote_port_(remote_port),
      ip_(ip),
      local_port_(local_port),
      fiber_acceptor_(io_service) {}

template <typename Demux>
void FibersToSockets<Demux>::start(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "microservice[stream_forwarder]: start "
                       "forwarding stream fiber from fiber port " << local_port_
                    << " to TCP <" << ip_ << ":" << remote_port_ << ">";

  FiberEndpoint ep(this->get_demux(), local_port_);
  fiber_acceptor_.bind(ep, ec);
  fiber_acceptor_.listen(boost::asio::socket_base::max_connections, ec);

  // Resolve the given address
  Tcp::resolver resolver(this->get_io_service());
  Tcp::resolver::query query(ip_, std::to_string(remote_port_));
  Tcp::resolver::iterator iterator(resolver.resolve(query, ec));
  if (ec) {
    SSF_LOG(kLogError)
        << "microservice[stream_forwarder]: cannot resolve remote endpoint";
    return;
  }

  remote_endpoint_ = *iterator;
  this->AsyncAcceptFibers();
}

template <typename Demux>
void FibersToSockets<Demux>::stop(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "microservice[stream_forwarder]: stopping";
  ec.assign(::error::success, ::error::get_ssf_category());

  fiber_acceptor_.close();
  manager_.stop_all();
}

template <typename Demux>
uint32_t FibersToSockets<Demux>::service_type_id() {
  return kFactoryId;
}

template <typename Demux>
void FibersToSockets<Demux>::AsyncAcceptFibers() {
  SSF_LOG(kLogTrace)
      << "microservice[stream_forwarder]: accepting new connections";

  FiberPtr new_connection = std::make_shared<Fiber>(
      this->get_io_service(), FiberEndpoint(this->get_demux(), 0));

  fiber_acceptor_.async_accept(
      *new_connection, boost::bind(&FibersToSockets::FiberAcceptHandler,
                                   this->SelfFromThis(), new_connection, _1));
}

template <typename Demux>
void FibersToSockets<Demux>::FiberAcceptHandler(
    FiberPtr fiber_connection, const boost::system::error_code& ec) {
  if (ec) {
    SSF_LOG(kLogDebug)
        << "microservice[stream_forwarder]: error accepting new connection: "
        << ec.message() << " " << ec.value();
    return;
  }

  if (fiber_acceptor_.is_open()) {
    this->AsyncAcceptFibers();
  }

  std::shared_ptr<Tcp::socket> socket =
      std::make_shared<Tcp::socket>(this->get_io_service());
  socket->async_connect(
      remote_endpoint_,
      boost::bind(&FibersToSockets::TcpSocketConnectHandler,
                  this->SelfFromThis(), socket, fiber_connection, _1));
}

template <typename Demux>
void FibersToSockets<Demux>::TcpSocketConnectHandler(
    std::shared_ptr<Tcp::socket> socket, FiberPtr fiber_connection,
    const boost::system::error_code& ec) {
  SSF_LOG(kLogTrace) << "microservice[stream_forwarder]: connect handler";

  if (ec) {
    SSF_LOG(kLogError)
        << "microservice[stream_forwarder]: error connecting to remote socket";
    fiber_connection->close();
    return;
  }

  auto session = SessionForwarder<Fiber, Tcp::socket>::create(
      &manager_, std::move(*fiber_connection), std::move(*socket));
  boost::system::error_code start_ec;
  manager_.start(session, start_ec);
  if (start_ec) {
    SSF_LOG(kLogError)
        << "microservice[stream_forwarder]: cannot start session";
    start_ec.clear();
    session->stop(start_ec);
  }
}

}  // fibers_to_sockets
}  // services
}  // ssf

#endif  // SSF_SERVICES_FIBERS_TO_SOCKETS_FIBERS_TO_SOCKETS_IPP_
