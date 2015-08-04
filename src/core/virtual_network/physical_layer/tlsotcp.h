#ifndef SSF_CORE_VIRTUAL_NETWORK_PHYSICAL_LAYER_TLSOTCP_H_
#define SSF_CORE_VIRTUAL_NETWORK_PHYSICAL_LAYER_TLSOTCP_H_

#include "core/virtual_network/cryptography_layer/basic_crypto_stream.h"
#include "core/virtual_network/cryptography_layer/tls/OpenSSL/impl.h"

#include "core/virtual_network/physical_layer/tcp.h"

namespace virtual_network {
namespace physical_layer {

using TLSboTCPPhysicalLayer = cryptography_layer::basic_CryptoStreamProtocol<
    tcp, cryptography_layer::buffered_tls>;

using TLSoTCPPhysicalLayer =
    cryptography_layer::basic_CryptoStreamProtocol<tcp,
                                                   cryptography_layer::tls>;

}  // physical_layer
}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_PHYSICAL_LAYER_TLSOTCP_H_
