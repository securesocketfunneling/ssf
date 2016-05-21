#ifndef SSF_SERVICES_PROCESS_PROCESS_SERVER_IPP_
#define SSF_SERVICES_PROCESS_PROCESS_SERVER_IPP_

#include <iostream>
#include <string>

#include <boost/bind.hpp>               // NOLINT
#include <boost/thread.hpp>             // NOLINT
#include <boost/system/error_code.hpp>  //NOLINT

#include <ssf/log/log.h>

namespace ssf {
namespace services {
namespace process {

template <typename Demux>
Server<Demux>::Server(boost::asio::io_service& io_service, demux& fiber_demux,
                      const local_port_type& port,
                      const std::string& binary_path,
                      const std::string& binary_args)
    : ssf::BaseService<Demux>::BaseService(io_service, fiber_demux),
      fiber_acceptor_(io_service),
      session_manager_(),
      new_connection_(io_service, endpoint(fiber_demux, 0)),
      local_port_(port),
      binary_path_(binary_path),
      binary_args_(binary_args) {}

template <typename Demux>
void Server<Demux>::start(boost::system::error_code& ec) {
  endpoint ep(this->get_demux(), local_port_);
  fiber_acceptor_.bind(ep, ec);

  if (ec) {
    SSF_LOG(kLogError)
        << "service[process]: fiber acceptor could not bind on port "
        << local_port_;
    return;
  }

  fiber_acceptor_.listen(boost::asio::socket_base::max_connections, ec);
  if (ec) {
    SSF_LOG(kLogError) << "service[process]: fiber acceptor could not listen";
    return;
  }

  if (!CheckBinaryPath()) {
    SSF_LOG(kLogError) << "service[process]: binary not found";
    ec.assign(::error::file_not_found, ::error::get_ssf_category());
    return;
  }

  SSF_LOG(kLogInfo) << "service[process]: starting server on port "
                    << local_port_;

  this->StartAccept();
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
      new_connection_,
      boost::bind(&Server::HandleAccept, this->SelfFromThis(), _1));
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
  ssf::BaseSessionPtr new_process_session = std::make_shared<session_impl>(
      &(this->session_manager_), std::move(this->new_connection_),
      binary_path_, binary_args_);
  boost::system::error_code e;
  this->session_manager_.start(new_process_session, e);

  this->StartAccept();
}

template <typename Demux>
void Server<Demux>::HandleStop() {
  fiber_acceptor_.close();
  session_manager_.stop_all();
}

template <typename Demux>
bool Server<Demux>::CheckBinaryPath() {
  std::ifstream binary_file(binary_path_);
  return binary_file.good();
}

}  // process
}  // services
}  // ssf

#endif  // SSF_SERVICES_PROCESS_PROCESS_SERVER_IPP_
