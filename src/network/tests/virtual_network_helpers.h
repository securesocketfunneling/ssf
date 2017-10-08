#ifndef SSF_TESTS_VIRTUAL_NETWORK_HELPERS_H_
#define SSF_TESTS_VIRTUAL_NETWORK_HELPERS_H_

#include <functional>

#include <boost/system/error_code.hpp>

#include "ssf/layer/basic_empty_stream.h"
#include "ssf/layer/basic_empty_datagram.h"
#include "ssf/layer/cryptography/tls/OpenSSL/impl.h"
#include "ssf/layer/cryptography/basic_crypto_stream.h"

#include "ssf/layer/parameters.h"

namespace tests {
namespace virtual_network_helpers {

template <class NextLayer>
using Layer = ssf::layer::VirtualEmptyStreamProtocol<NextLayer>;

template <class NextLayer>
using TLSLayer = ssf::layer::cryptography::basic_CryptoStreamProtocol<
    NextLayer, ssf::layer::cryptography::buffered_tls>;

template <class NextLayer>
using DatagramLayer = ssf::layer::VirtualEmptyDatagramProtocol<NextLayer>;

typedef std::function<void(const boost::system::error_code&, std::size_t)>
    SendHandler;
typedef std::function<void(const boost::system::error_code&, std::size_t)>
    ReceiveHandler;
typedef std::function<void(const boost::system::error_code&)> ConnectHandler;
typedef std::function<void(const boost::system::error_code&)> AcceptHandler;

template <class Buffer>
void ResetBuffer(Buffer* p_buffer, typename Buffer::value_type value,
                 std::size_t length, bool identical = true) {
  if (p_buffer->size() > length) {
    length = p_buffer->size();
  }
  for (size_t i = 0; i < length; ++i) {
    (*p_buffer)[i] = identical ? value : (value + i) % 256;
  }
}

template <class Buffer>
void ResetBuffer(Buffer* p_buffer, typename Buffer::value_type value,
                 bool identical = true) {
  ResetBuffer(p_buffer, value, p_buffer->size(), identical);
}

template <class Buffer>
bool CheckBuffers(const Buffer& buffer1, const Buffer& buffer2,
                  std::size_t length) {
  if (buffer1.size() < length || buffer2.size() < length) {
    return false;
  }

  for (std::size_t i = 0; i < length; ++i) {
    if (buffer1[i] != buffer2[i]) {
      return false;
    }
  }

  return true;
}

ssf::layer::LayerParameters GetServerTLSParametersAsFile();

ssf::layer::LayerParameters GetClientTLSParametersAsFile();

ssf::layer::LayerParameters GetDefaultServerParameters();

ssf::layer::LayerParameters GetServerTLSParametersAsBuffer();

ssf::layer::LayerParameters GetClientTLSParametersAsBuffer();

}  // virtual_network_helpers
}  // tests

#endif  // SSF_TESTS_VIRTUAL_NETWORK_HELPERS_H_
