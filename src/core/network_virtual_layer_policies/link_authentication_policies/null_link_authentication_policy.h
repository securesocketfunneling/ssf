#ifndef SSF_CORE_NETWORK_VIRTUAL_LAYER_POLICIES_LINK_AUTHENTICATION_POLICIES_NULL_LINK_AUTHENTICATION_POLICY_H_
#define SSF_CORE_NETWORK_VIRTUAL_LAYER_POLICIES_LINK_AUTHENTICATION_POLICIES_NULL_LINK_AUTHENTICATION_POLICY_H_

#include <cstdint>

#include <list>
#include <memory>
#include <map>
#include <string>
#include <functional>

#include <boost/system/error_code.hpp>
#include <boost/bind.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/buffer.hpp>

namespace ssf {

template <typename SocketType>
class NullLinkAuthenticationPolicy {
 private:
  typedef SocketType socket_type;
  typedef std::shared_ptr<socket_type> p_socket_type;
  typedef std::shared_ptr<uint32_t> p_uint32_t;
  typedef std::map<std::string, std::string> Parameters;

  typedef std::function<void(const Parameters&, p_socket_type, const boost::system::error_code&)>
      callback_type;

public:
 void GetCredentials(Parameters& parameters, callback_type callback,
                     p_socket_type p_socket) {
    auto p_value = std::make_shared<uint32_t>(0);
    if (parameters["local"] == "true") {
      callback(parameters, p_socket, boost::system::error_code());
    } else {
      boost::asio::async_write(
          *p_socket, boost::asio::buffer(p_value.get(), sizeof(*p_value)),
          boost::bind(
              &NullLinkAuthenticationPolicy::RemoteConnectionEstablishedHandler,
              this, parameters, callback, p_value, p_socket, _1, _2));
    }
  }

  void SetCredentials(const Parameters& parameters, callback_type callback,
                       p_socket_type p_socket) {
    auto p_value = std::make_shared<uint32_t>(0);

    boost::asio::async_read(
        *p_socket, boost::asio::buffer(p_value.get(), sizeof(*p_value)),
        boost::bind(
            &NullLinkAuthenticationPolicy::NextConnectionEstablishedHandler,
            this, parameters, callback, p_value, p_socket, _1, _2));
  }

  std::list<std::string> GetBouncingNodes(const Parameters& parameters) {
    if (parameters.count("bouncing_nodes")) {
      auto serialized_list = parameters.find("bouncing_nodes")->second;

      std::istringstream istrs(serialized_list);
      boost::archive::text_iarchive ar(istrs);
      std::list<std::string> bouncing_nodes;

      try {
        ar >> BOOST_SERIALIZATION_NVP(bouncing_nodes);
        return bouncing_nodes;
      } catch (const std::exception&) {
        return std::list<std::string>();
      }
    } else {
      return std::list<std::string>();
    }
  }

  std::string PopEndpointString(std::list<std::string>& bouncing_nodes) {
    if (bouncing_nodes.size()) {
      auto first = bouncing_nodes.front();
      bouncing_nodes.pop_front();
      return first;
    } else {
      return "";
    }
  }

private:
 void RemoteConnectionEstablishedHandler(const Parameters& parameters,
                                            callback_type callback,
                                            p_uint32_t p_value,
                                            p_socket_type p_socket,
                                            const boost::system::error_code& ec,
                                            size_t length) {
    callback(parameters, p_socket, ec);
  }

  void NextConnectionEstablishedHandler(const Parameters& parameters,
                                           callback_type callback,
                                           p_uint32_t p_value,
                                           p_socket_type p_socket,
                                           const boost::system::error_code& ec,
                                           size_t length) {

    callback(parameters, p_socket, ec);
  }
};
}  // ssf

#endif  // SSF_CORE_NETWORK_VIRTUAL_LAYER_POLICIES_LINK_AUTHENTICATION_POLICIES_NULL_LINK_AUTHENTICATION_POLICY_H_
