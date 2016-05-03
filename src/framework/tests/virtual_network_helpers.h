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
using TLSLayer =
    ssf::layer::cryptography::basic_CryptoStreamProtocol<
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

ssf::layer::LayerParameters tls_server_parameters = {
  {"ca_src", "file"},
  {"crt_src", "file"},
  {"key_src", "file"},
  {"dhparam_src", "file"},
  {"ca_file", "./certs/trusted/ca.crt"},
  {"crt_file", "./certs/certificate.crt"},
  {"key_file", "./certs/private.key"},
  {"dhparam_file", "./certs/dh4096.pem"}};

ssf::layer::LayerParameters tls_client_parameters = {
  {"ca_src", "file"},
  {"crt_src", "file"},
  {"key_src", "file"},
  {"ca_file", "./certs/trusted/ca.crt"},
  {"crt_file", "./certs/certificate.crt"},
  {"key_file", "./certs/private.key"}};

ssf::layer::LayerParameters tls_default_server_parameters = {
    {"default", "true"}};

}  // virtual_network_helpers
}  // tests

#endif  // SSF_TESTS_VIRTUAL_NETWORK_HELPERS_H_
