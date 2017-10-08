#ifndef SSF_SERVICES_DATAGRAMS_TO_FIBERS_DATAGRAMS_TO_FIBERS_IPP_
#define SSF_SERVICES_DATAGRAMS_TO_FIBERS_DATAGRAMS_TO_FIBERS_IPP_

#include <ssf/log/log.h>

#include <ssf/network/session_forwarder.h>

namespace ssf {
namespace services {
namespace datagrams_to_fibers {

template <typename Demux>
DatagramsToFibers<Demux>::DatagramsToFibers(boost::asio::io_service& io_service,
                                            Demux& fiber_demux,
                                            const std::string& local_addr,
                                            LocalPortType local_port,
                                            RemotePortType remote_port)
    : ssf::BaseService<Demux>::BaseService(io_service, fiber_demux),
      local_addr_(local_addr),
      local_port_(local_port),
      remote_port_(remote_port),
      socket_(io_service),
      from_endpoint_(),
      to_endpoint_(fiber_demux, remote_port),
      p_udp_operator_(UdpOperator::Create(socket_)) {}

template <typename Demux>
void DatagramsToFibers<Demux>::start(boost::system::error_code& ec) {

  Udp::resolver resolver(socket_.get_io_service());
  Udp::resolver::query query(local_addr_, std::to_string(local_port_));
  auto ep_it = resolver.resolve(query, ec);

  if (ec) {
    SSF_LOG(kLogError) << "microservice[datagram_listener]: could not "
                          "resolve query <" << local_addr_ << ":" << local_port_
                       << ">";
    return;
  }

  from_endpoint_ = *ep_it;

  boost::system::error_code close_ec;
  socket_.open(Udp::v4(), ec);
  if (ec) {
    SSF_LOG(kLogError)
        << "microservice[datagram_listener]: could not open UDP socket";
    socket_.close(close_ec);
    return;
  }

  boost::asio::socket_base::reuse_address reuse_address_option(true);
  socket_.set_option(reuse_address_option, ec);
  if (ec) {
    SSF_LOG(kLogError) << "microservice[datagram_listener]: could not set "
                          "reuse address option";
    socket_.close(close_ec);
    return;
  }

  socket_.bind(from_endpoint_, ec);
  if (ec) {
    SSF_LOG(kLogError)
        << "microservice[datagram_listener]: could not bind UDP socket <"
        << local_addr_ << ":" << local_port_ << ">";
    socket_.close(close_ec);
    return;
  }

  SSF_LOG(kLogInfo)
      << "microservice[datagram_listener]: forward UDP datagrams from <"
      << local_addr_ << ":" << local_port_ << "> to fiber port "
      << remote_port_;

  this->AsyncReceiveDatagram();
}

template <typename Demux>
void DatagramsToFibers<Demux>::stop(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "microservice[datagram_listener]: stopping";
  socket_.close(ec);

  if (ec) {
    SSF_LOG(kLogDebug) << "microservice[datagram_listener]: error on stop "
                       << ec.message();
  }

  p_udp_operator_->StopAll();
}

template <typename Demux>
uint32_t DatagramsToFibers<Demux>::service_type_id() {
  return kFactoryId;
}

template <typename Demux>
void DatagramsToFibers<Demux>::AsyncReceiveDatagram() {
  SSF_LOG(kLogTrace)
      << "microservice[datagram_listener]: receiving new datagram";

  socket_.async_receive_from(
      boost::asio::buffer(working_buffer_), from_endpoint_,
      std::bind(&DatagramsToFibers::OnSocketDatagramReceive, this,
                this->shared_from_this(), std::placeholders::_1,
                std::placeholders::_2));
}

template <typename Demux>
void DatagramsToFibers<Demux>::OnSocketDatagramReceive(
    BaseServicePtr self, const boost::system::error_code& ec, size_t length) {
  if (ec) {
    SSF_LOG(kLogDebug)
        << "microservice[datagram_listener]: error receiving datagram: "
        << ec.message() << " " << ec.value();
    return;
  }

  auto already_in = p_udp_operator_->Feed(
      from_endpoint_, boost::asio::buffer(working_buffer_), length);

  if (!already_in) {
    FiberDatagram left(this->get_demux().get_io_service(),
                       FiberEndpoint(this->get_demux(), 0));
    p_udp_operator_->AddLink(std::move(left), from_endpoint_, to_endpoint_,
                             this->get_io_service());
    p_udp_operator_->Feed(from_endpoint_, boost::asio::buffer(working_buffer_),
                          length);
  }

  this->AsyncReceiveDatagram();
}

}  // datagrams_to_fibers
}  // services
}  // ssf

#endif  // SSF_SERVICES_DATAGRAMS_TO_FIBERS_DATAGRAMS_TO_FIBERS_IPP_
