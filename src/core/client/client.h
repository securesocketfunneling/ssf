#ifndef SSF_CORE_CLIENT_CLIENT_H_
#define SSF_CORE_CLIENT_CLIENT_H_

#include <functional>
#include <future>
#include <map>
#include <string>

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include "common/boost/fiber/stream_fiber.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"

#include "core/async_engine.h"
#include "core/service_manager/service_manager.h"

#include "services/initialisation.h"
#include "services/user_services/base_user_service.h"

namespace ssf {
template <class NetworkProtocol,
          template <class> class TransportVirtualLayerPolicy>
class SSFClient
    : public TransportVirtualLayerPolicy<typename NetworkProtocol::socket> {
 private:
  using NetworkSocket = typename NetworkProtocol::socket;
  using NetworkSocketPtr = std::shared_ptr<NetworkSocket>;
  using NetworkEndpoint = typename NetworkProtocol::endpoint;
  using NetworkResolver = typename NetworkProtocol::resolver;
  using NetworkQuery = typename NetworkProtocol::resolver::query;

 public:
  using Demux = boost::asio::fiber::basic_fiber_demux<NetworkSocket>;
  using BaseUserServicePtr =
      typename ssf::services::BaseUserService<Demux>::BaseUserServicePtr;
  using ClientCallback =
      std::function<void(ssf::services::initialisation::type,
                         BaseUserServicePtr, const boost::system::error_code&)>;

 public:
  SSFClient(std::vector<BaseUserServicePtr> user_services,
            ClientCallback callback);

  ~SSFClient();

  void Run(const NetworkQuery& query, boost::system::error_code& ec);

  void Stop();
  
  boost::asio::io_service& get_io_service();

 private:
  void AsyncWaitIntTerm(const boost::system::error_code& ec, int signum);
  
  void NetworkToTransport(const boost::system::error_code& ec,
                          NetworkSocketPtr p_socket);

  void DoSSFStart(NetworkSocketPtr p_socket,
                  const boost::system::error_code& ec);

  void DoFiberize(NetworkSocketPtr p_socket, boost::system::error_code& ec);

  void OnDemuxClose();

  void Notify(ssf::services::initialisation::type type,
              BaseUserServicePtr p_user_service, boost::system::error_code ec);

 private:
  AsyncEngine async_engine_;
  
  Demux fiber_demux_;

  std::vector<BaseUserServicePtr> user_services_;

  ClientCallback callback_;
};

}  // ssf

#include "core/client/client.ipp"

#endif  // SSF_CORE_CLIENT_CLIENT_H_
