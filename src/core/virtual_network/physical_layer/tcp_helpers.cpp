#include "core/virtual_network/physical_layer/tcp_helpers.h"

#include <string>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "common/error/error.h"
#include "common/utils/map_helpers.h"

#include "core/virtual_network/parameters.h"

namespace virtual_network {
namespace physical_layer {
namespace detail {

boost::asio::ip::tcp::endpoint make_tcp_endpoint(
    boost::asio::io_service& io_service, const LayerParameters& parameters,
    boost::system::error_code& ec) {
  auto addr = helpers::GetField<std::string>("addr", parameters);
  auto port = helpers::GetField<std::string>("port", parameters);

  if (port != "") {
    if (addr != "") {
      boost::asio::ip::tcp::resolver resolver(io_service);
      boost::asio::ip::tcp::resolver::query query(addr, port);
      boost::asio::ip::tcp::resolver::iterator iterator(
          resolver.resolve(query, ec));

      if (!ec) {
        return boost::asio::ip::tcp::endpoint(*iterator);
      }
    } else {
      try {
        return boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),
                                              (uint16_t)std::stoul(port));
      } catch (const std::exception&) {
        ec.assign(ssf::error::bad_address, ssf::error::get_ssf_category());
        return boost::asio::ip::tcp::endpoint();
      }
    }
  }

  return boost::asio::ip::tcp::endpoint();
}

}  // detail
}  // physical_layer
}  // virtual_network
