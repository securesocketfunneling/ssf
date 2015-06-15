#ifndef SSF_TESTS_VIRTUAL_NETWORK_HELPERS_H_
#define SSF_TESTS_VIRTUAL_NETWORK_HELPERS_H_

#include <cstdint>

#include <array>
#include <list>
#include <map>
#include <string>
#include <vector>

#include <boost/system/error_code.hpp>

#include "core/virtual_network/physical_layer/tcp.h"
#include "core/virtual_network/physical_layer/udp.h"

#include "core/virtual_network/basic_empty_stream.h"
#include "core/virtual_network/basic_empty_datagram.h"
#include "core/virtual_network/cryptography_layer/ssl.h"
#include "core/virtual_network/cryptography_layer/basic_empty_ssl_stream.h"

namespace tests {
struct virtual_network_helpers {
  template <class NextLayer>
  using Layer = virtual_network::VirtualEmptyStreamProtocol<NextLayer>;

  template <class NextLayer>
  using SSLLayer =
      virtual_network::cryptography_layer::VirtualEmptySSLStreamProtocol<
          virtual_network::cryptography_layer::buffered_ssl<NextLayer>>;

  typedef Layer<virtual_network::physical_layer::tcp> TCPPhysicalProtocol;
  typedef SSLLayer<virtual_network::physical_layer::tcp> SSLPhysicalProtocol;

  typedef std::function<void(const boost::system::error_code&, std::size_t)>
      SendHandler;
  typedef std::function<void(const boost::system::error_code&, std::size_t)>
      ReceiveHandler;
  typedef std::function<void(const boost::system::error_code&)> ConnectHandler;
  typedef std::function<void(const boost::system::error_code&)> AcceptHandler;

  typedef std::map<std::string, std::string> Parameters;
  typedef std::list<Parameters> ParametersList;

  template <class Buffer>
  static void ResetBuffer(Buffer* p_buffer, typename Buffer::value_type value) {
    for (size_t i = 0; i < p_buffer->size(); ++i) {
      (*p_buffer)[i] = value;
    }
  }

  template <class Buffer>
  static bool CheckBuffers(const Buffer& buffer1, const Buffer& buffer2) {
    if (buffer1.size() != buffer2.size()) {
      return false;
    }

    for (std::size_t i = 0; i < buffer1.size(); ++i) {
      if (buffer1[i] != buffer2[i]) {
        return false;
      }
    }

    return true;
  }
};

}  // tests

#endif  // SSF_TESTS_VIRTUAL_NETWORK_HELPERS_H_
