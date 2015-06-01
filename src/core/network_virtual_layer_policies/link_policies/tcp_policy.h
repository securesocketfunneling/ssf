#ifndef SSF_CORE_NETWORK_VIRTUAL_LAYER_POLICIES_TCP_POLICY_H_
#define SSF_CORE_NETWORK_VIRTUAL_LAYER_POLICIES_TCP_POLICY_H_

#include <functional>
#include <memory>
#include <string>

#include <boost/system/error_code.hpp>
#include <boost/bind.hpp>
#include <boost/log/trivial.hpp>
#include <boost/asio/connect.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>

#include "versions.h"
#include "common/config/config.h"
#include "common/error/error.h"

namespace ssf {

/// A connect policy for a client to use plain TCP as transport
class TCPPolicy {
 public:
   /// Policy required transport socket type
  typedef boost::asio::ip::tcp::socket socket_type;

  /// Policy required pointer type
  typedef std::shared_ptr<socket_type> p_socket_type;

  typedef boost::asio::ip::tcp::acceptor acceptor_type;
  typedef std::shared_ptr<acceptor_type> p_acceptor_type;

 private:
  typedef std::function<void(p_socket_type, const boost::system::error_code&)>
      connect_callback_type;
  typedef std::function<void(p_socket_type)> accept_callback_type;
  typedef std::map<std::string, std::string> Parameters;

 public:
  virtual ~TCPPolicy() {}

  TCPPolicy(boost::asio::io_service& io_service,
            const ssf::Config& ssf_config)
      : io_service_(io_service) { }

  void establish_link(const Parameters& parameters,
                      connect_callback_type connect_callback) {

    auto addr = get_remote_addr(parameters);
    auto port = get_remote_port(parameters);

    BOOST_LOG_TRIVIAL(info) << "link: connecting " << addr << " " << port;
    if (addr != "" && port != "") {
      boost::asio::ip::tcp::resolver resolver(io_service_);
      boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(),
        addr, port);
      boost::asio::ip::tcp::resolver::iterator iterator(resolver.resolve(query));

      auto p_socket = std::make_shared<socket_type>(io_service_);

      boost::asio::async_connect(
        *p_socket, iterator,
        boost::bind(&TCPPolicy::connected_handler, this, p_socket,
                    connect_callback, _1));
    } else {
      boost::system::error_code ec(ssf::error::invalid_argument,
                                   ssf::error::get_ssf_category());
      to_next_layer_handler(p_socket_type(nullptr), connect_callback, ec);
    }
  }

  /// Accept a new connection
  void accept_links(p_acceptor_type p_acceptor,
                    accept_callback_type accept_callback) {

    if (!p_acceptor->is_open()) {
      return;
    }

    auto p_socket = std::make_shared<socket_type>(io_service_);

    BOOST_LOG_TRIVIAL(trace) << "link: accepting";
    p_acceptor->async_accept(
        *p_socket, boost::bind(&TCPPolicy::accepted_handler, this, p_acceptor,
                               p_socket, accept_callback, _1));
  }

  void close_link(socket_type& socket) {
    boost::system::error_code ec;
    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket.close(ec);
  }

 private:
  /// Write link version
  void connected_handler(p_socket_type p_socket,
                          connect_callback_type connect_callback,
                          const boost::system::error_code& ec) {
    if (!ec) {
      auto p_version = std::make_shared<uint32_t>(GetVersion());
      /*boost::asio::async_write(*p_socket,
                               boost::asio::buffer(p_version.get(), sizeof(*p_version)),
                               boost::bind(&TCPPolicy::write_version_handler, this,
                                           p_socket, p_version, connect_callback, _1, _2));*/
      write_version_handler(p_socket, p_version, connect_callback, ec, 0);
    } else {
      BOOST_LOG_TRIVIAL(error) << "link: connection failed " << ec.message();
      close_link(*p_socket);
      this->to_next_layer_handler(nullptr, connect_callback, ec);
    }
  }

  void write_version_handler(p_socket_type p_socket,
                             std::shared_ptr<uint32_t> p_version,
                             connect_callback_type connect_callback,
                             const boost::system::error_code& ec,
                             size_t bytes_transferred) {
    if (!ec) {
      BOOST_LOG_TRIVIAL(info) << "link: connected";
      this->to_next_layer_handler(p_socket, connect_callback, ec);
    } else {
      BOOST_LOG_TRIVIAL(error) << "link: connection failed " << ec.message();
      close_link(*p_socket);
      this->to_next_layer_handler(nullptr, connect_callback, ec);
    }
  }

  void to_next_layer_handler(p_socket_type p_socket,
                             connect_callback_type connect_callback,
                             const boost::system::error_code& ec) {
    io_service_.post(boost::bind(connect_callback, p_socket, ec));
  }

  /// Read link version
  void accepted_handler(p_acceptor_type p_acceptor, p_socket_type p_socket,
                        accept_callback_type accept_callback,
                        const boost::system::error_code& ec) {
    if (!ec) {
      /*std::shared_ptr<uint32_t> p_version = std::make_shared<uint32_t>();
      boost::asio::async_read(*p_socket,
                              boost::asio::buffer(p_version.get(), sizeof(*p_version)),
                              boost::bind(&TCPPolicy::read_version_handler, this,
                                          p_socket, p_version, accept_callback, _1, _2));*/
      std::shared_ptr<uint32_t> p_version =
          std::make_shared<uint32_t>(GetVersion());
      read_version_handler(p_socket, p_version, accept_callback, ec, 0);
      this->accept_links(p_acceptor, accept_callback);
    } else {
      BOOST_LOG_TRIVIAL(error) << "link: NOT Authenticated " << ec.message();
      this->close_link(*p_socket);
      this->to_next_layer_handler(nullptr, accept_callback);
    }
  }

  void read_version_handler(p_socket_type p_socket,
                            std::shared_ptr<uint32_t> p_version,
                            accept_callback_type accept_callback,
                            const boost::system::error_code& ec,
                            size_t bytes_transferred) {
    auto version_supported = IsVersionSupported(*p_version);
    if (!ec && version_supported) {
      this->to_next_layer_handler(p_socket, accept_callback);
      BOOST_LOG_TRIVIAL(trace) << "link: authenticated";
    } else {
      if (!version_supported) {
        BOOST_LOG_TRIVIAL(error) << "link: version NOT supported "
                                 << *p_version;
      }
      if (ec) {
        BOOST_LOG_TRIVIAL(error) << "link: error on read version "
                                 << "ec : " << ec.message();
      }
      BOOST_LOG_TRIVIAL(error) << "link: NOT Authenticated ";
      close_link(*p_socket);
      this->to_next_layer_handler(nullptr, accept_callback);
    }
  }

  void to_next_layer_handler(p_socket_type p_socket,
                             accept_callback_type accept_callback) {
    io_service_.post(boost::bind(accept_callback, p_socket));
  }

  std::string get_remote_addr(const Parameters& parameters) {
    if (parameters.count("remote_addr")) {
      return parameters.find("remote_addr")->second;
    } else {
      return "";
    }
  }

  std::string get_remote_port(const Parameters& parameters) {
    if (parameters.count("remote_port")) {
      return parameters.find("remote_port")->second;
    } else {
      return "";
    }
  }

  uint32_t GetVersion() {
    uint32_t version = ssf::versions::Versions::major;
    version = version << 8;

    version |= ssf::versions::Versions::minor;
    version = version << 8;

    version |= ssf::versions::Versions::security;
    version = version << 8;

    version |= uint8_t(boost::archive::BOOST_ARCHIVE_VERSION());

    return version;
  }

  bool IsVersionSupported(uint32_t input_version) {
    boost::archive::library_version_type serialization(input_version &
                                                       0x000000FF);
    input_version = input_version >> 8;

    uint8_t security = (input_version & 0x000000FF);
    input_version = input_version >> 8;

    uint8_t minor = (input_version & 0x000000FF);
    input_version = input_version >> 8;

    uint8_t major = (input_version & 0x000000FF);

    return (major == ssf::versions::Versions::major) &&
      (minor == ssf::versions::Versions::minor) &&
      (security == ssf::versions::Versions::security) &&
      (serialization == boost::archive::BOOST_ARCHIVE_VERSION());
  }

 private:
  boost::asio::io_service& io_service_;
};

}  // ssf

#endif  // SSF_CORE_NETWORK_VIRTUAL_LAYER_POLICIES_TCP_POLICY_H_
