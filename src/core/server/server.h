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

#include "core/factories/service_factory.h"
#include "core/async_runner.h"
#include "core/service_manager/service_manager.h"

#include "services/admin/admin.h"
#include "services/base_service.h"
#include "services/copy_file/file_to_fiber/file_to_fiber.h"
#include "services/copy_file/fiber_to_file/fiber_to_file.h"
#include "services/datagrams_to_fibers/datagrams_to_fibers.h"
#include "services/fibers_to_sockets/fibers_to_sockets.h"
#include "services/fibers_to_datagrams/fibers_to_datagrams.h"
#include "services/sockets_to_fibers/sockets_to_fibers.h"
#include "services/socks/socks_server.h"

namespace ssf {
template <class NetworkProtocol,
          template <class> class TransportVirtualLayerPolicy>
class SSFServer
    : public AsyncRunner,
      public TransportVirtualLayerPolicy<typename NetworkProtocol::socket> {
 private:
  using network_socket_type = typename NetworkProtocol::socket;
  using p_network_socket_type = std::shared_ptr<network_socket_type>;
  using network_endpoint_type = typename NetworkProtocol::endpoint;
  using p_network_endpoint_type = std::shared_ptr<network_endpoint_type>;
  using network_resolver_type = typename NetworkProtocol::resolver;
  using network_query_type = typename NetworkProtocol::resolver::query;
  using network_acceptor_type = typename NetworkProtocol::acceptor;

  using worker_ptr = std::unique_ptr<boost::asio::io_service::work>;

 public:
  typedef boost::asio::fiber::basic_fiber_demux<network_socket_type> demux;

 private:
  typedef std::shared_ptr<demux> p_demux;

  typedef std::shared_ptr<ServiceManager<demux>> p_ServiceManager;

  typedef std::set<p_demux> demux_set;
  typedef std::map<p_demux, p_network_socket_type> socket_map;
  typedef std::map<p_demux, p_ServiceManager> service_manager_map;

 public:
  SSFServer();

  ~SSFServer();

  void Run(const network_query_type& query, boost::system::error_code& ec);
  void Stop();

 private:
  void AsyncAcceptConnection();
  void NetworkToTransport(const boost::system::error_code& ec,
                          p_network_socket_type p_socket);
  void AddDemux(p_demux p_fiber_demux, p_ServiceManager p_service_manager);
  void DoSSFStart(p_network_socket_type p_socket,
                  const boost::system::error_code& ec);
  void DoFiberize(p_network_socket_type p_socket,
                  boost::system::error_code& ec);
  void RemoveDemux(p_demux p_fiber_demux);
  void RemoveAllDemuxes();

 private:
  worker_ptr p_worker_;
  network_acceptor_type network_acceptor_;

  demux_set p_fiber_demuxes_;
  service_manager_map p_service_managers_;

  boost::recursive_mutex storage_mutex_;
};

}  // ssf

#include "core/server/server.ipp"

#endif  // SSF_CORE_SERVER_SERVER_H_
