# Proxy test configuration

* Copy file 'proxy.json.dist' into 'proxy.json'
* Fill options with correct values:
  - target_addr: address of your machine reachable from proxy
  - target_addr: port of test server (9000 by default)
  - proxy_addr: address of http proxy
  - proxy_port: port of http proxy
  - username: username for proxy authentication
  - password: password for proxy authentication
* Run `cmake ..` at project build directory
* Build and run `proxy_layer_tests`
