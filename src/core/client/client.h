#ifndef SSF_CORE_CLIENT_CLIENT_H_
#define SSF_CORE_CLIENT_CLIENT_H_

#include <memory>
#include <set>
#include <map>
#include <string>
#include <functional>

#include <boost/asio.hpp>

#include "common/boost/fiber/stream_fiber.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"
#include "common/config/config.h"

#include "services/initialisation.h"
#include "services/admin/admin.h"
#include "services/sockets_to_fibers/sockets_to_fibers.h"
#include "services/fibers_to_sockets/fibers_to_sockets.h"
#include "services/fibers_to_datagrams/fibers_to_datagrams.h"
#include "services/datagrams_to_fibers/datagrams_to_fibers.h"
#include "services/socks/socks_server.h"

#include "services/admin/requests/create_service_request.h"

#include "core/service_manager/service_manager.h"
#include "services/base_service.h"


namespace ssf {
template <typename PhysicalVirtualLayer,
          template <class> class LinkAuthenticationPolicy,
          template <class, template <class> class>
            class NetworkVirtualLayerPolicy,
          template <class> class TransportVirtualLayerPolicy>
class SSFClient : public NetworkVirtualLayerPolicy<PhysicalVirtualLayer,
                                                   LinkAuthenticationPolicy>,
                  public TransportVirtualLayerPolicy<
                    typename PhysicalVirtualLayer::socket_type> {

 private:
  typedef typename PhysicalVirtualLayer::socket_type socket_type;
  typedef typename PhysicalVirtualLayer::p_socket_type p_socket_type;
  typedef std::map<std::string, std::string> Parameters;

  typedef std::vector<boost::system::error_code> vector_error_code_type;

 public:
  typedef boost::asio::fiber::basic_fiber_demux<socket_type> demux;
  typedef typename ssf::services::BaseUserService<demux>::BaseUserServicePtr
    BaseUserServicePtr;
  typedef std::function<
    void(ssf::services::initialisation::type,
         BaseUserServicePtr,
         const boost::system::error_code&)>
      client_callback_type;

 public:
  SSFClient(boost::asio::io_service& io_service, const std::string& remote_addr,
            const std::string& port, const ssf::Config& ssf_config,
            std::vector<BaseUserServicePtr> user_services,
            client_callback_type callback);

  void Run(Parameters& parameters);

  void Stop();

 private:
  void DoSSFStart(p_socket_type p_socket, const boost::system::error_code& ec);

  void DoFiberize(p_socket_type p_socket, boost::system::error_code& ec);

  void OnDemuxClose();

  void NetworkToTransport(p_socket_type p_socket, vector_error_code_type v_ec);
  bool PrintErrorVector(vector_error_code_type v_ec);

  void Notify(ssf::services::initialisation::type type,
              BaseUserServicePtr p_user_service,
              boost::system::error_code ec) {
    if (callback_) {
      io_service_.post(
          boost::bind(callback_, std::move(type), p_user_service, std::move(ec)));
    }
  }

 private:
  boost::asio::io_service& io_service_;
  demux fiber_demux_;

  std::vector<BaseUserServicePtr> user_services_;

  std::string remote_addr_;
  std::string remote_port_;

  client_callback_type callback_;
};
}  // ssf

#include "core/client/client.ipp"

#endif  // SSF_CORE_CLIENT_CLIENT_H_
