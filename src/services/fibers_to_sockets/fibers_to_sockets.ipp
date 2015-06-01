#ifndef SSF_SERVICES_FIBERS_TO_SOCKETS_FIBERS_TO_SOCKETS_IPP_
#define SSF_SERVICES_FIBERS_TO_SOCKETS_FIBERS_TO_SOCKETS_IPP_

#include <boost/log/trivial.hpp>

#include "common/error/error.h"
#include "common/network/session_forwarder.h"

namespace ssf { namespace services { namespace fibers_to_sockets {

template <typename Demux>
RemoteForwarderService<Demux>::RemoteForwarderService(
    boost::asio::io_service& io_service, demux& fiber_demux,
    local_port_type local_port, const std::string& ip, uint16_t remote_port)
    : ssf::BaseService<Demux>::BaseService(io_service, fiber_demux),
      remote_port_(remote_port),
      ip_(ip),
      local_port_(local_port),
      fiber_acceptor_(io_service),
      fiber_(io_service),
      socket_(io_service) {}

template <typename Demux>
void RemoteForwarderService<Demux>::start(boost::system::error_code& ec) {
  BOOST_LOG_TRIVIAL(info) << "service fibers to sockets: starting relay on local port tcp "
                          << local_port_;

  endpoint ep(this->get_demux(), local_port_);
  fiber_acceptor_.bind(ep, ec);
  fiber_acceptor_.listen(boost::asio::socket_base::max_connections, ec);

  // Resolve the given address
  boost::asio::ip::tcp::resolver resolver(this->get_io_service());
  boost::asio::ip::tcp::resolver::query query(ip_,
                                              std::to_string(remote_port_));
  boost::asio::ip::tcp::resolver::iterator iterator(
      resolver.resolve(query, ec));

  if (!ec) {
    endpoint_ = *iterator;
    this->StartAcceptFibers();
  }
}

template <typename Demux>
void RemoteForwarderService<Demux>::stop(boost::system::error_code& ec) {
  BOOST_LOG_TRIVIAL(info) << "service fibers to sockets: stopping";
  ec.assign(ssf::error::success,
            ssf::error::get_ssf_category());

  fiber_acceptor_.close();
  manager_.stop_all();
}

template <typename Demux>
uint32_t RemoteForwarderService<Demux>::service_type_id() {
  return factory_id;
}

template <typename Demux>
void RemoteForwarderService<Demux>::StartAcceptFibers() {
  BOOST_LOG_TRIVIAL(trace) << "service fibers to sockets: accepting new clients";

  fiber_acceptor_.async_accept(
      fiber_,
      Then(&RemoteForwarderService::FiberAcceptHandler, this->SelfFromThis()));
}

template <typename Demux>
void RemoteForwarderService<Demux>::FiberAcceptHandler(const boost::system::error_code& ec) {
  BOOST_LOG_TRIVIAL(trace) << "service fibers to sockets: accept handler";

  if (!fiber_acceptor_.is_open()) {
    return;
  }

  if (!ec) {
    socket_.async_connect(
        endpoint_,
        Then(&RemoteForwarderService::SocketConnectHandler, this->SelfFromThis()));
  }
}

template <typename Demux>
void RemoteForwarderService<Demux>::SocketConnectHandler(const boost::system::error_code& ec) {
  BOOST_LOG_TRIVIAL(trace) << "service fibers to sockets: connect handler";

  if (!ec) {
    auto session = SessionForwarder<fiber, socket>::create(
        &manager_, std::move(fiber_), std::move(socket_));
    boost::system::error_code e;
    manager_.start(session, e);
  }

  this->StartAcceptFibers();
}

}  // fibers_to_sockets
}  // services
}  // ssf

#endif  // SSF_SERVICES_FIBERS_TO_SOCKETS_FIBERS_TO_SOCKETS_IPP_
