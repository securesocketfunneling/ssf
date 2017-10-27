#ifndef SSF_SERVICES_FIBERS_TO_DATAGRAMS_FIBERS_TO_DATAGRAMS_IPP_
#define SSF_SERVICES_FIBERS_TO_DATAGRAMS_FIBERS_TO_DATAGRAMS_IPP_

#include <ssf/log/log.h>

#include "common/error/error.h"

namespace ssf {
namespace services {
namespace fibers_to_datagrams {
template <typename Demux>
FibersToDatagrams<Demux>::FibersToDatagrams(boost::asio::io_service& io_service,
                                            Demux& fiber_demux,
                                            LocalPortType local_port,
                                            const std::string& ip,
                                            RemotePortType remote_port)
    : ssf::BaseService<Demux>::BaseService(io_service, fiber_demux),
      remote_port_(remote_port),
      ip_(ip),
      local_port_(local_port),
      fiber_(io_service),
      from_endpoint_(fiber_demux, 0),
      p_udp_operator_(UdpOperator::Create(fiber_)) {}

template <typename Demux>
void FibersToDatagrams<Demux>::start(boost::system::error_code& ec) {
  fiber_.bind(FiberEndpoint(this->get_demux(), local_port_), ec);
  if (ec) {
    SSF_LOG(kLogError) << "microservice[datagram_forwarder]: cannot bind "
                          "datagram fiber to port "
                       << local_port_;
    return;
  }

  // Resolve the given address
  Udp::resolver resolver(this->get_io_service());
  Udp::resolver::query query(ip_, std::to_string(remote_port_));
  Udp::resolver::iterator iterator(resolver.resolve(query, ec));
  if (ec) {
    SSF_LOG(kLogError) << "microservice[datagram_forwarder]: cannot resolve "
                          "remote UDP endpoint <"
                       << ip_ << ":" << remote_port_ << ">";
    return;
  }

  to_endpoint_ = *iterator;

  SSF_LOG(kLogInfo) << "microservice[datagram_forwarder]: forward "
                       "fiber datagrams from fiber port "
                    << local_port_ << " to <" << ip_ << ":" << remote_port_
                    << ">";

  this->AsyncReceiveDatagram();
}

template <typename Demux>
void FibersToDatagrams<Demux>::stop(boost::system::error_code& ec) {
  SSF_LOG(kLogDebug) << "microservice[datagram_forwarder]: stop";
  ec.assign(::error::success, ::error::get_ssf_category());

  fiber_.close();
  p_udp_operator_->StopAll();
}

template <typename Demux>
uint32_t FibersToDatagrams<Demux>::service_type_id() {
  return kFactoryId;
}

template <typename Demux>
void FibersToDatagrams<Demux>::AsyncReceiveDatagram() {
  fiber_.async_receive_from(
      boost::asio::buffer(working_buffer_), from_endpoint_,
      std::bind(&FibersToDatagrams::OnFiberDatagramReceive, this,
                this->shared_from_this(), std::placeholders::_1,
                std::placeholders::_2));
}

template <typename Demux>
void FibersToDatagrams<Demux>::OnFiberDatagramReceive(
    BaseServicePtr self, const boost::system::error_code& ec, size_t length) {
  if (ec) {
    SSF_LOG(kLogDebug)
        << "microservice[datagram_forwarder]: error receiving datagram: "
        << ec.message() << " " << ec.value();
    return;
  }

  auto already_in = p_udp_operator_->Feed(
      from_endpoint_, boost::asio::buffer(working_buffer_), length);

  if (!already_in) {
    Udp::socket left(this->get_io_service(), Udp::endpoint(Udp::v4(), 0));
    p_udp_operator_->AddLink(std::move(left), from_endpoint_, to_endpoint_,
                             this->get_io_service());
    p_udp_operator_->Feed(from_endpoint_, boost::asio::buffer(working_buffer_),
                          length);
  }

  this->AsyncReceiveDatagram();
}

}  // fibers_to_datagrams
}  // services
}  // ssf

#endif  // SSF_SERVICES_FIBERS_TO_DATAGRAMS_FIBERS_TO_DATAGRAMS_IPP_
