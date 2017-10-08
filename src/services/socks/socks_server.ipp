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
                                Demux& fiber_demux, const LocalPortType& port)
    : ssf::BaseService<Demux>::BaseService(io_service, fiber_demux),
      fiber_acceptor_(io_service),
      session_manager_(),
      local_port_(port) {
  // The init_ec will be returned when start() is called
  // fiber_acceptor_.open();
  FiberEndpoint ep(this->get_demux(), port);
  fiber_acceptor_.bind(ep, init_ec_);
  fiber_acceptor_.listen(boost::asio::socket_base::max_connections, init_ec_);
}

template <typename Demux>
void SocksServer<Demux>::start(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "microservice[socks]: start server on fiber port "
                    << local_port_;
  ec = init_ec_;
  if (ec) {
    return;
  }

  this->AsyncAcceptFiber();
}

template <typename Demux>
void SocksServer<Demux>::stop(boost::system::error_code& ec) {
  ec.assign(boost::system::errc::success, boost::system::system_category());

  SSF_LOG(kLogInfo) << "microservice[socks]: stopping server";
  this->HandleStop();
}

template <typename Demux>
uint32_t SocksServer<Demux>::service_type_id() {
  return kFactoryId;
}

template <typename Demux>
void SocksServer<Demux>::StopSession(BaseSessionPtr session,
                                     boost::system::error_code& ec) {
  session_manager_.stop(session, ec);
}

template <typename Demux>
void SocksServer<Demux>::AsyncAcceptFiber() {
  SSF_LOG(kLogTrace) << "microservice[socks]: accepting new connections";
  FiberPtr new_connection = std::make_shared<Fiber>(
      this->get_io_service(), FiberEndpoint(this->get_demux(), 0));

  fiber_acceptor_.async_accept(
      *new_connection,
      std::bind(&SocksServer<Demux>::FiberAcceptHandler, this->SelfFromThis(),
                new_connection, std::placeholders::_1));
}

template <typename Demux>
void SocksServer<Demux>::FiberAcceptHandler(
    FiberPtr fiber_connection, const boost::system::error_code& ec) {
  if (ec) {
    SSF_LOG(kLogDebug)
        << "microservice[socks]: error accepting new connection: "
        << ec.message() << " " << ec.value();
    return;
  }

  if (fiber_acceptor_.is_open()) {
    this->AsyncAcceptFiber();
  }

  std::shared_ptr<Version> p_version(new Version());

  auto self = this->SelfFromThis();
  auto start_handler =
      [this, self, p_version, fiber_connection](
          boost::system::error_code read_ec, std::size_t) {
        if (read_ec) {
          SSF_LOG(kLogError)
              << "microservice[socks]: error reading protocol version: "
              << read_ec.message() << " " << read_ec.value();
          fiber_connection->close();
          return;
        }

        switch (p_version->Number()) {
          case static_cast<uint8_t>(ssf::network::Socks::Version::kV4): {
            SSF_LOG(kLogTrace) << "microservice[socks]: version accepted: v4";
            ssf::BaseSessionPtr new_socks_session =
                std::make_shared<v4::Session<Demux> >(
                    this->SelfFromThis(), std::move(*fiber_connection));
            boost::system::error_code e;
            session_manager_.start(new_socks_session, e);
            break;
          }
          case static_cast<uint8_t>(ssf::network::Socks::Version::kV5): {
            SSF_LOG(kLogTrace) << "microservice[socks]: version accepted: v5";
            ssf::BaseSessionPtr new_socks_session =
                std::make_shared<v5::Session<Demux> >(
                    this->SelfFromThis(), std::move(*fiber_connection));
            boost::system::error_code e;
            session_manager_.start(new_socks_session, e);
            break;
          }
          default: {
            SSF_LOG(kLogError)
                << "microservice[socks]: protocol not supported yet: "
                << p_version->Number();
            fiber_connection->close();
            break;
          }
        }
      };

  // Read the version field of the SOCKS header
  boost::asio::async_read(*fiber_connection, p_version->MutBuffer(),
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
