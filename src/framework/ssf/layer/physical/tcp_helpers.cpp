#include "ssf/layer/physical/tcp_helpers.h"

#include <string>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "ssf/error/error.h"

#include "ssf/layer/parameters.h"

#include "ssf/layer/physical/host.h"

namespace ssf {
namespace layer {
namespace physical {
namespace detail {

boost::asio::ip::tcp::endpoint make_tcp_endpoint(
    boost::asio::io_service& io_service, const LayerParameters& parameters,
    boost::system::error_code& ec) {
  Host host(parameters);

  if (!host.port().empty()) {
    if (!host.addr().empty()) {
      boost::asio::ip::tcp::resolver resolver(io_service);
      boost::asio::ip::tcp::resolver::query query(host.addr(), host.port());
      boost::asio::ip::tcp::resolver::iterator iterator(
          resolver.resolve(query, ec));

      if (!ec) {
        return boost::asio::ip::tcp::endpoint(*iterator);
      }
    } else {
      try {
        return boost::asio::ip::tcp::endpoint(
            boost::asio::ip::tcp::v4(), (uint16_t)std::stoul(host.port()));
      } catch (const std::exception&) {
        ec.assign(ssf::error::bad_address, ssf::error::get_ssf_category());
        return boost::asio::ip::tcp::endpoint();
      }
    }
  }

  return boost::asio::ip::tcp::endpoint();
}

}  // detail
}  // physical
}  // layer
}  // ssf
