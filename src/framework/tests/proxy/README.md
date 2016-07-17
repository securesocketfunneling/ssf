# Proxy test configuration

* Copy file 'proxy.json.dist' into 'proxy.json'
* Fill options with correct values:
  - target_host: address of your machine reachable from proxy
  - target_port: port of test server (9000 by default)
  - proxy_host: address of http proxy
  - proxy_port: port of http proxy
  - username: username for proxy authentication
  - password: password for proxy authentication
  - domain: user domain (NTLM and Negotiate auth on Windows only)
  - reuse_ntlm: reuse current computer user credentials to authenticate with proxy NTLM auth (SSO)
  "reuse_kerb": reuse current computer user credentials (Kerberos ticket) to authenticate with proxy Negotiate auth (SSO)
* Run `cmake ..` at project build directory
* Build and run `proxy_layer_tests`
