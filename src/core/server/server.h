#ifndef SSF_CORE_SERVER_SERVER_H_
#define SSF_CORE_SERVER_SERVER_H_

#include <set>
#include <map>

#include <boost/asio.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "common/boost/fiber/stream_fiber.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"

#include "common/config/config.h"

#include "services/admin/admin.h"
#include "services/socks/socks_server.h"
#include "services/fibers_to_sockets/fibers_to_sockets.h"
#include "services/sockets_to_fibers/sockets_to_fibers.h"
#include "services/fibers_to_datagrams/fibers_to_datagrams.h"
#include "services/datagrams_to_fibers/datagrams_to_fibers.h"
#include "services/copy_file/file_to_fiber/file_to_fiber.h"
#include "services/copy_file/fiber_to_file/fiber_to_file.h"

#include "core/service_manager/service_manager.h"
#include "services/base_service.h"

#include "core/factories/service_factory.h"

namespace ssf {
template <typename PhysicalVirtualLayer,
          template <class> class LinkAuthenticationPolicy,
          template <class, template <class> class>
          class NetworkVirtualLayerPolicy,
          template <class> class TransportVirtualLayerPolicy>
class SSFServer : public NetworkVirtualLayerPolicy<PhysicalVirtualLayer,
                                                   LinkAuthenticationPolicy>,
                  public TransportVirtualLayerPolicy<
                      typename PhysicalVirtualLayer::socket_type> {
 private:
  typedef typename PhysicalVirtualLayer::socket_type socket_type;
  typedef typename PhysicalVirtualLayer::p_socket_type p_socket_type;

  typedef std::vector<boost::system::error_code> vector_error_code_type;

 public:
  typedef boost::asio::fiber::basic_fiber_demux<socket_type> demux;

 private:
  typedef std::shared_ptr<demux> p_demux;

  typedef std::shared_ptr<ServiceManager<demux>> p_ServiceManager;

  typedef std::set<p_demux> demux_set;
  typedef std::map<p_demux, p_socket_type> socket_map;
  typedef std::map<p_demux, p_ServiceManager> service_manager_map;

 public:
  SSFServer(boost::asio::io_service& io_service, const ssf::Config& ssf_config,
            uint16_t local_port);

  void Run();
  void Stop();

 private:
  void AddDemux(p_demux p_fiber_demux, p_ServiceManager p_service_manager);
  void RemoveDemux(p_demux p_fiber_demux);
  void RemoveAllDemuxes();
  void DoSSFStart(p_socket_type p_socket, const boost::system::error_code& ec);
  void DoFiberize(p_socket_type p_socket, boost::system::error_code& ec);
  void NetworkToTransport(p_socket_type p_socket, vector_error_code_type v_ec);

 private:
  boost::asio::io_service& io_service_;

  uint16_t local_port_;

  demux_set p_fiber_demuxes_;
  service_manager_map p_service_managers_;

  boost::recursive_mutex storage_mutex_;
};

}  // ssf

#include "core/server/server.ipp"

#endif  // SSF_CORE_SERVER_SERVER_H_
