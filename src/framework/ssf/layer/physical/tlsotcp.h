#ifndef SSF_LAYER_PHYSICAL_TLSOTCP_H_
#define SSF_LAYER_PHYSICAL_TLSOTCP_H_

#include "ssf/layer/cryptography/basic_crypto_stream.h"
#include "ssf/layer/cryptography/tls/OpenSSL/impl.h"

#include "ssf/layer/physical/tcp.h"

namespace ssf {
namespace layer {
namespace physical {

using TLSboTCPPhysicalLayer = cryptography::basic_CryptoStreamProtocol<
    tcp, cryptography::buffered_tls>;

using TLSoTCPPhysicalLayer =
    cryptography::basic_CryptoStreamProtocol<tcp, cryptography::tls>;

}  // physical
}  // layer
}  // ssf

#endif  // SSF_LAYER_PHYSICAL_TLSOTCP_H_
