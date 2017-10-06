#ifndef SSF_SERVICES_PROCESS_PROCESS_SERVER_IPP_
#define SSF_SERVICES_PROCESS_PROCESS_SERVER_IPP_

#include <functional>
#include <iostream>
#include <string>

#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

namespace ssf {
namespace services {
namespace process {

template <typename Demux>
Server<Demux>::Server(boost::asio::io_service& io_service, Demux& fiber_demux,
                      const LocalPortType& port, const std::string& binary_path,
                      const std::string& binary_args)
    : ssf::BaseService<Demux>::BaseService(io_service, fiber_demux),
      fiber_acceptor_(io_service),
      session_manager_(),
      local_port_(port),
      binary_path_(binary_path),
      binary_args_(binary_args) {}

template <typename Demux>
void Server<Demux>::start(boost::system::error_code& ec) {
  FiberEndpoint ep(this->get_demux(), local_port_);
  fiber_acceptor_.bind(ep, ec);

  if (ec) {
    SSF_LOG(kLogError)
        << "microservice[shell]: fiber acceptor could not bind on port "
        << local_port_;
    return;
  }

  fiber_acceptor_.listen(boost::asio::socket_base::max_connections, ec);
  if (ec) {
    SSF_LOG(kLogError)
        << "microservice[shell]: fiber acceptor could not listen";
    return;
  }

  if (!CheckBinaryPath()) {
    SSF_LOG(kLogError) << "microservice[shell]: binary not found";
    ec.assign(::error::file_not_found, ::error::get_ssf_category());
    return;
  }

  SSF_LOG(kLogInfo) << "microservice[shell]: start server on fiber port "
                    << local_port_;

  this->AsyncAcceptFiber();
}

template <typename Demux>
void Server<Demux>::stop(boost::system::error_code& ec) {
  ec.assign(boost::system::errc::success, boost::system::system_category());

  SSF_LOG(kLogInfo) << "microservice[shell]: stop server";
  this->HandleStop();
}

template <typename Demux>
uint32_t Server<Demux>::service_type_id() {
  return kFactoryId;
}

template <typename Demux>
void Server<Demux>::StopSession(BaseSessionPtr session,
                                boost::system::error_code& ec) {
  session_manager_.stop(session, ec);
}

template <typename Demux>
void Server<Demux>::AsyncAcceptFiber() {
  SSF_LOG(kLogTrace) << "microservice[shell]: accepting new session";
  FiberPtr new_connection = std::make_shared<Fiber>(
      this->get_io_service(), FiberEndpoint(this->get_demux(), 0));

  fiber_acceptor_.async_accept(
      *new_connection,
      std::bind(&Server::FiberAcceptHandler, this->SelfFromThis(),
                new_connection, std::placeholders::_1));
}

template <typename Demux>
void Server<Demux>::FiberAcceptHandler(FiberPtr new_connection,
                                       const boost::system::error_code& ec) {
  if (ec) {
    SSF_LOG(kLogDebug)
        << "microservice[shell]: error accepting new connections: "
        << ec.message() << " " << ec.value();
    return;
  }

  SSF_LOG(kLogInfo) << "microservice[shell]: start session";

  this->AsyncAcceptFiber();

  ssf::BaseSessionPtr new_process_session = std::make_shared<SessionImpl>(
      this->SelfFromThis(), std::move(*new_connection), binary_path_,
      binary_args_);
  boost::system::error_code e;
  session_manager_.start(new_process_session, e);
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
