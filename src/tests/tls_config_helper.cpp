#include "tests/tls_config_helper.h"

#include "common/config/config.h"

#include "ca_cert_def.h"
#include "client_cert_def.h"
#include "client_key_def.h"
#include "server_cert_def.h"
#include "server_dh_param_def.h"
#include "server_key_def.h"

namespace ssf {
namespace tests {

std::string GetCaCert() { return TEST_CA_CERT; }

std::string GetClientCert() { return TEST_CLIENT_CERT; }

std::string GetClientKey() { return TEST_CLIENT_KEY; }

std::string GetServerCert() { return TEST_SERVER_CERT; }

std::string GetServerKey() { return TEST_SERVER_KEY; }

std::string GetServerDhParam() { return TEST_SERVER_DH_PARAM; }

void SetClientTlsConfig(ssf::config::Config* config) {
  config->tls().mutable_ca_cert()->Set(config::TlsParam::Type::kBuffer,
                                       GetCaCert());
  config->tls().mutable_cert()->Set(config::TlsParam::Type::kBuffer,
                                    GetClientCert());
  config->tls().mutable_key()->Set(config::TlsParam::Type::kBuffer,
                                    GetClientKey());
}

void SetServerTlsConfig(ssf::config::Config* config) {
  config->tls().mutable_ca_cert()->Set(config::TlsParam::Type::kBuffer,
                                       GetCaCert());
  config->tls().mutable_cert()->Set(config::TlsParam::Type::kBuffer,
                                    GetServerCert());
  config->tls().mutable_key()->Set(config::TlsParam::Type::kBuffer,
                                    GetServerKey());
  config->tls().mutable_dh()->Set(config::TlsParam::Type::kBuffer,
                                  GetServerDhParam());
}

}  // tests
}  // ssf