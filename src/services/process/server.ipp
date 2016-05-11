#ifndef SSF_SERVICES_PROCESS_PROCESS_SERVER_IPP_
#define SSF_SERVICES_PROCESS_PROCESS_SERVER_IPP_

#include <string>

#include <boost/bind.hpp>               // NOLINT
#include <boost/thread.hpp>             // NOLINT
#include <boost/system/error_code.hpp>  //NOLINT

#include <ssf/log/log.h>

#include "services/process/session.h"

namespace ssf {
namespace services {
namespace process {

template <typename Demux>
Server<Demux>::Server(boost::asio::io_service& io_service, demux& fiber_demux,
                      const local_port_type& port)
    : ssf::BaseService<Demux>::BaseService(io_service, fiber_demux),
      fiber_acceptor_(io_service),
      session_manager_(),
      new_connection_(io_service, endpoint(fiber_demux, 0)),
      local_port_(port) {
  endpoint ep(this->get_demux(), port);
  fiber_acceptor_.bind(ep, init_ec_);
  fiber_acceptor_.listen(boost::asio::socket_base::max_connections, init_ec_);
}

template <typename Demux>
void Server<Demux>::start(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "service[process]: starting server on port "
                    << local_port_;
  ec = init_ec_;

  if (!init_ec_) {
    this->StartAccept();
  }
}

template <typename Demux>
void Server<Demux>::stop(boost::system::error_code& ec) {
  ec.assign(boost::system::errc::success, boost::system::system_category());

  SSF_LOG(kLogInfo) << "service[process]: stopping server";
  this->HandleStop();
}

template <typename Demux>
uint32_t Server<Demux>::service_type_id() {
  return factory_id;
}

template <typename Demux>
void Server<Demux>::StartAccept() {
  SSF_LOG(kLogTrace) << "service[process]: accept new session";
  fiber_acceptor_.async_accept(
      new_connection_, Then(&Server::HandleAccept, this->SelfFromThis()));
}

template <typename Demux>
void Server<Demux>::HandleAccept(const boost::system::error_code& ec) {
  SSF_LOG(kLogTrace) << "service[process]: HandleAccept";

  if (!fiber_acceptor_.is_open()) {
    return;
  }

  if (ec) {
    SSF_LOG(kLogError) << "service[process]: error accepting new connection: "
                       << ec.message() << " " << ec.value();
    this->StartAccept();
  }

  SSF_LOG(kLogInfo) << "service[process]: start session";
  ssf::BaseSessionPtr new_process_session =
      std::make_shared<ssf::process::Session<Demux> >(
          &(this->session_manager_), std::move(this->new_connection_));
  boost::system::error_code e;
  this->session_manager_.start(new_socks_session, e);

  this->StartAccept();
}

template <typename Demux>
void Server<Demux>::HandleStop() {
  fiber_acceptor_.close();
  session_manager_.stop_all();
}

}  // process
}  // services
}  // ssf

#endif  // SSF_SERVICES_PROCESS_PROCESS_SERVER_IPP_
