#ifndef SSF_CORE_CLIENT_SESSION_H_
#define SSF_CORE_CLIENT_SESSION_H_

#include <functional>
#include <memory>
#include <vector>

#include <boost/system/error_code.hpp>

#include "common/boost/fiber/stream_fiber.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"

#include "core/client/status.h"
#include "core/service_manager/service_manager.h"

#include "services/user_services/base_user_service.h"

namespace ssf {

template <class NetworkProtocol,
          template <class> class TransportVirtualLayerPolicy>
class Session
    : public TransportVirtualLayerPolicy<typename NetworkProtocol::socket>,
      public std::enable_shared_from_this<
          Session<NetworkProtocol, TransportVirtualLayerPolicy>> {
 public:
  using SessionPtr = typename std::shared_ptr<
      Session<NetworkProtocol, TransportVirtualLayerPolicy>>;
  using NetworkSocket = typename NetworkProtocol::socket;
  using NetworkSocketPtr = std::shared_ptr<NetworkSocket>;
  using NetworkEndpoint = typename NetworkProtocol::endpoint;
  using NetworkResolver = typename NetworkProtocol::resolver;
  using NetworkQuery = typename NetworkProtocol::resolver::query;
  using Demux = boost::asio::fiber::basic_fiber_demux<NetworkSocket>;
  using BaseUserServicePtr =
      typename ssf::services::BaseUserService<Demux>::BaseUserServicePtr;
  using OnStatusCb = std::function<void(Status)>;
  using OnUserServiceStatusCb =
      std::function<void(BaseUserServicePtr, const boost::system::error_code&)>;

 public:
  static std::shared_ptr<Session> Create(
      boost::asio::io_service& io_service,
      std::vector<BaseUserServicePtr> user_services,
      const ssf::config::Services& services_config, OnStatusCb on_status,
      OnUserServiceStatusCb on_user_service_status,
      boost::system::error_code& ec);

  ~Session();

  void Start(const NetworkQuery& query, boost::system::error_code& ec);

  void Stop(boost::system::error_code& ec);

  Demux& GetDemux() { return fiber_demux_; }

  boost::asio::io_service& get_io_service() { return io_service_; }

 private:
  Session(boost::asio::io_service& io_service,
          const std::vector<BaseUserServicePtr>& user_services,
          const ssf::config::Services& services_config, OnStatusCb on_status,
          OnUserServiceStatusCb on_user_service_status);

  void NetworkToTransport(const boost::system::error_code& ec);

  void DoSSFStart(NetworkSocketPtr p_socket,
                  const boost::system::error_code& ec);

  void DoFiberize(NetworkSocketPtr p_socket, boost::system::error_code& ec);

  void OnDemuxClose();

  void UpdateStatus(Status status);

 private:
  boost::asio::io_service& io_service_;
  NetworkSocketPtr p_socket_;
  std::vector<BaseUserServicePtr> user_services_;
  ssf::config::Services services_config_;
  ServiceManagerPtr<Demux> p_service_manager_;
  Demux fiber_demux_;
  bool stopped_;
  Status status_;
  OnStatusCb on_status_;
  OnUserServiceStatusCb on_user_service_status_;
};

}  // ssf

#include "core/client/session.ipp"

#endif  // SSF_CORE_CLIENT_SESSION_H_
