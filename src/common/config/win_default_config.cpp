#include "common/config/config.h"

const char* ssf::config::Config::default_config_ = R"RAWSTRING(
{
  "ssf": {
    "arguments": "",
    "circuit": [],
    "tls": {
      "ca_cert_path": "./certs/trusted/ca.crt",
      "cert_path": "./certs/certificate.crt",
      "key_path": "./certs/private.key",
      "key_password": "",
      "dh_path": "./certs/dh4096.pem",
      "cipher_alg": "DHE-RSA-AES256-GCM-SHA384"
    },
    "http_proxy": {
      "host": "",
      "port": "",
      "user_agent": "",
      "credentials": {
        "username": "",
        "password": "",
        "domain": "",
        "reuse_ntlm": true,
        "reuse_nego": true
      }
    },
    "socks_proxy": {
      "version": 5,
      "host": "",
      "port": "1080"
    },
    "services": {
      "datagram_forwarder": { "enable": true },
      "datagram_listener": {
        "enable": true,
        "gateway_ports": false
      },
      "stream_forwarder": { "enable": true },
      "stream_listener": {
        "enable": true,
        "gateway_ports": false
      },
      "copy": { "enable": false },
      "shell": {
        "enable": false,
        "path": "C:\\windows\\system32\\cmd.exe",
        "args": ""
      },
      "socks": { "enable": true }
    }
  }
}
)RAWSTRING";
