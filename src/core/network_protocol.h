#ifndef SSF_CORE_NETWORK_PROTOCOL_H_
#define SSF_CORE_NETWORK_PROTOCOL_H_

#include <string>

#include <ssf/layer/data_link/circuit_helpers.h>
#include <ssf/layer/data_link/basic_circuit_protocol.h>
#include <ssf/layer/data_link/simple_circuit_policy.h>
#include <ssf/layer/parameters.h>
#include <ssf/layer/physical/tcp.h>
#include <ssf/layer/physical/tlsotcp.h>

#include "common/config/config.h"

namespace ssf {
namespace network {
using Query = ssf::layer::ParameterStack;
using CircuitBouncers = std::list<std::string>;

template <class Layer>
using TLSboLayer = ssf::layer::cryptography::basic_CryptoStreamProtocol<
    Layer, ssf::layer::cryptography::buffered_tls>;

using TLSPhysicalProtocol = ssf::layer::physical::TLSboTCPPhysicalLayer;
using CircuitTLSProtocol = ssf::layer::data_link::basic_CircuitProtocol<
    TLSPhysicalProtocol, ssf::layer::data_link::CircuitPolicy>;
using TLSoCircuitTLSProtocol = TLSboLayer<CircuitTLSProtocol>;

using PhysicalProtocol = ssf::layer::physical::TCPPhysicalLayer;
using CircuitProtocol = ssf::layer::data_link::basic_CircuitProtocol<
    PhysicalProtocol, ssf::layer::data_link::CircuitPolicy>;

using PlainProtocol = CircuitProtocol;
using TLSProtocol = CircuitTLSProtocol;
using FullTLSProtocol = TLSoCircuitTLSProtocol;

#ifdef TLS_OVER_TCP_LINK
using Protocol = FullTLSProtocol;
#elif TCP_ONLY_LINK
using Protocol = PlainProtocol;
#endif

Query GenerateClientQuery(const std::string& remote_addr,
                          const std::string& remote_port,
                          const ssf::Config& ssf_config,
                          const CircuitBouncers& bouncers);

Query GenerateServerQuery(const std::string& remote_addr,
                          const std::string& remote_port,
                          const ssf::Config& ssf_config);

Query GenerateClientTCPQuery(const std::string& remote_addr,
                             const std::string& remote_port,
                             const CircuitBouncers& nodes);

Query GenerateClientTLSQuery(const std::string& remote_addr,
                             const std::string& remote_port,
                             const ssf::Config& ssf_config,
                             const CircuitBouncers& nodes);

Query GenerateServerTCPQuery(const std::string& remote_addr,
                             const std::string& remote_port);

Query GenerateServerTLSQuery(const std::string& remote_addr,
                             const std::string& remote_port,
                             const ssf::Config& ssf_config);

}  // network
}  // ssf

#endif  // SSF_CORE_NETWORK_PROTOCOL_H_
