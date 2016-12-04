#ifndef SSF_SERVICES_SOCKS_SOCKS_SERVER_IPP_
#define SSF_SERVICES_SOCKS_SOCKS_SERVER_IPP_

#include <string>

#include <ssf/network/socks/socks.h>

#include <ssf/utils/enum.h>

#include "services/socks/socks_version.h"
#include "services/socks/v4/session.h"
#include "services/socks/v5/session.h"

namespace ssf {
namespace services {
namespace socks {

template <typename Demux>
SocksServer<Demux>::SocksServer(boost::asio::io_service& io_service,
                                demux& fiber_demux, const local_port_type& port)
    : ssf::BaseService<Demux>::BaseService(io_service, fiber_demux),
      fiber_acceptor_(io_service),
      session_manager_(),
      new_connection_(io_service, endpoint(fiber_demux, 0)),
      local_port_(port) {
  // The init_ec will be returned when start() is called
  // fiber_acceptor_.open();
  endpoint ep(this->get_demux(), port);
  fiber_acceptor_.bind(ep, init_ec_);
  fiber_acceptor_.listen(boost::asio::socket_base::max_connections, init_ec_);
}

template <typename Demux>
void SocksServer<Demux>::start(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "microservice[socks]: start server on fiber port "
                    << local_port_;
  ec = init_ec_;

  if (!init_ec_) {
    this->StartAccept();
  }
}

template <typename Demux>
void SocksServer<Demux>::stop(boost::system::error_code& ec) {
  ec.assign(boost::system::errc::success, boost::system::system_category());

  SSF_LOG(kLogInfo) << "microservice[socks]: stopping server";
  this->HandleStop();
}

template <typename Demux>
uint32_t SocksServer<Demux>::service_type_id() {
  return factory_id;
}

template <typename Demux>
void SocksServer<Demux>::StartAccept() {
  fiber_acceptor_.async_accept(
      new_connection_, Then(&SocksServer::HandleAccept, this->SelfFromThis()));
}

template <typename Demux>
void SocksServer<Demux>::HandleAccept(const boost::system::error_code& ec) {
  SSF_LOG(kLogTrace) << "microservice[socks]: HandleAccept";

  if (!fiber_acceptor_.is_open()) {
    return;
  }

  if (ec) {
    SSF_LOG(kLogError)
        << "microservice[socks]: error accepting new connection: "
        << ec.message() << " " << ec.value();
    this->StartAccept();
    return;
  }

  std::shared_ptr<Version> p_version(new Version());

  auto self = this->SelfFromThis();
  auto start_handler =
      [this, self, p_version](boost::system::error_code read_ec, std::size_t) {
        if (read_ec) {
          SSF_LOG(kLogError)
              << "microservice[socks]: error reading protocol version: "
              << read_ec.message() << " " << read_ec.value();
          fiber fib = std::move(new_connection_);
          StartAccept();
          return;
        }

        switch (p_version->Number()) {
          case static_cast<uint8_t>(ssf::network::Socks::Version::kV4): {
            SSF_LOG(kLogTrace) << "microservice[socks]: version accepted: v4";
            ssf::BaseSessionPtr new_socks_session =
                std::make_shared<ssf::socks::v4::Session<Demux> >(
                    &(this->session_manager_),
                    std::move(this->new_connection_));
            boost::system::error_code e;
            session_manager_.start(new_socks_session, e);
            StartAccept();
            break;
          }
          case static_cast<uint8_t>(ssf::network::Socks::Version::kV5): {
            SSF_LOG(kLogTrace) << "microservice[socks]: version accepted: v5";
            ssf::BaseSessionPtr new_socks_session =
                std::make_shared<ssf::socks::v5::Session<Demux> >(
                    &(this->session_manager_),
                    std::move(this->new_connection_));
            boost::system::error_code e;
            session_manager_.start(new_socks_session, e);
            StartAccept();
            break;
          }
          default: {
            SSF_LOG(kLogError)
                << "microservice[socks]: protocol not supported yet: "
                << p_version->Number();
            new_connection_.close();
            fiber fib = std::move(new_connection_);
            StartAccept();
            break;
          }
        }
      };

  // Read the version field of the SOCKS header
  boost::asio::async_read(new_connection_, p_version->MutBuffer(),
                          start_handler);
}

template <typename Demux>
void SocksServer<Demux>::HandleStop() {
  fiber_acceptor_.close();
  session_manager_.stop_all();
}

}  // socks
}  // services
}  // ssf

#endif  // SSF_SERVICES_SOCKS_SOCKS_SERVER_IPP_
