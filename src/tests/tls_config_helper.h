#ifndef TESTS_TLS_CONFIG_HELPER_H_
#define TESTS_TLS_CONFIG_HELPER_H_

#include <string>

namespace ssf {

namespace config {
class Config;
}  // config

namespace tests {

std::string GetCaCert();
std::string GetClientCert();
std::string GetClientKey();
std::string GetServerCert();
std::string GetServerKey();
std::string GetServerDhParam();

void SetClientTlsConfig(ssf::config::Config* config);
void SetServerTlsConfig(ssf::config::Config* config);

}  // tests
}  // ssf

#endif  // TESTS_TLS_CONFIG_HELPER_H_