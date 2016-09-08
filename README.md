# Secure Socket Funneling

Secure Socket Funneling (SSF) is a network tool and toolkit.

It provides simple and efficient ways to forward data from multiple sockets (TCP or UDP) through a single secure TLS link to a remote computer.

SSF is cross platform (Windows, Linux, OSX) and shipped as standalone executables.

Features:
* Local and remote TCP port forwarding
* Local and remote UDP port forwarding
* Local and remote SOCKS server
* Local and remote shell through socket
* Native relay protocol
* TLS connection with strongest cipher-suites

[Download prebuilt binaries](https://securesocketfunneling.github.io/ssf/#download)

[Documentation](https://securesocketfunneling.github.io/ssf/)

## How to use

### Standard command line

#### Client command line

```plaintext
ssfc[.exe] [options] host

Basic options:
  -h [ --help ]                         Produce help message
  -v [ --verbosity ] level (=info)      Verbosity:
                                          critical|error|warning|info|debug|trace
  -q [ --quiet ]                        Do not display log

Local options:
  -c [ --config ] config_file_path      Set config file
  -b [ --circuit ] circuit_file_path    Set circuit file
  -p [ --port ] port (=8011)            Set remote SSF server port
  -g [ --gateway-ports ]                Allow gateway ports. At connection, client will be allowed to specify
                                        listening network interface on every services
  -S [ --status ]                       Display microservices status (on/off)

Supported service commands:
  -Y [ --remote-shell ] [[rem_ip]:]rem_port
                                        Open a port server side, each connection to that port launches a
                                        shell client side with I/O forwarded from/to the socket (shell microservice
                                        must be enabled client side prior to use)
  -F [ --remote-socks ] [[rem_ip]:]rem_port
                                        Run a SOCKS proxy on localhost accessible from server [[rem_ip]:]rem_port
  -X [ --shell ] [[loc_ip]:]loc_port
                                        Open a port on the client side, each connection to that port launches a
                                        shell server side with I/O forwarded to/from the socket (shell microservice
                                        must be enabled server side prior to use)
  -D [ --socks ] [[loc_ip]:]loc_port
                                        Run a SOCKS proxy on remote host accessible from client [[loc_ip]:]loc_port
  -L [ --tcp-forward ] [[loc_ip]:]loc_port:dest_ip:dest_port
                                        Forward TCP client [[loc_ip]:]port to dest_ip:dest_port from server
  -R [ --tcp-remote-forward ] [[rem_ip]:]rem_port:dest_ip:dest_port
                                        Forward TCP server [[rem_ip]:]rem_port to target dest_ip:dest_port from client
  -U [ --udp-forward ] [[loc_ip]:]loc_port:dest_ip:dest_port
                                        Forward UDP client [[loc_ip]:]loc_port to target dest_ip:dest_port from server
  -V [ --udp-remote-forward ] [[rem_ip]:]rem_port:dest_ip:dest_port
                                        Forward UDP server [[rem_ip]:]rem_port to dest_ip:dest_port from client
```

#### Server command line

```plaintext
ssfs[.exe] [options] [host]

Basic options:
  -h [ --help ]                         Produce help message
  -v [ --verbosity ] level (=info)      Verbosity:
                                          critical|error|warning|info|debug|trace
  -q [ --quiet ]                        Do not display log

Local options:
  -c [ --config ] config_file_path      Set config file
  -p [ --port ] port (=8011)            Set local SSF server port
  -R [ --relay-only ]                   Server will only relay connections
  -H [ --host ] host                    Set host
  -g [ --gateway-ports ]                Allow gateway ports. At connection, client will be allowed to specify listening
                                        network interface on every services
  -S [ --status ]                       Display microservices status (on/off)
```

#### Client example

Client will open port 9000 locally and wait SOCKS requests to be transferred to
server **192.168.0.1:8000**

```plaintext
ssfc[.exe] -D 9000 -b bounce.txt -c config.json -p 8000 192.168.0.1
```

#### Server example

Server will listen on all network interfaces on port **8011**

```plaintext
ssfs[.exe]
```

Server will listen on **192.168.0.1:9000**

```plaintext
ssfs[.exe] -p 9000 192.168.0.1
```

### Copy command line

Copy feature must be enabled both on client and server before usage.

Config file example:

```
"ssf": {
  "services": {
    "file_copy": { "enable": true }
  }
}
```

#### Command line

```plaintext
ssfcp[.exe] [options] [host@]/absolute/path/file [[host@]/absolute/path/file]

Basic options:
  -h [ --help ]                       Produce help message
  -v [ --verbosity ] level (=info)    Verbosity:
                                        critical|error|warning|info|debug|trace
  -q [ --quiet ]                      Do not display log

Local options:
  -c [ --config ] config_file_path    Set config file
  -b [ --circuit ] circuit_file_path  Set circuit file
  -p [ --port ] port (=8011)          Set remote SSF server port

Copy options:
  -t [ --stdin ]                      Input will be stdin
```

#### Copy from local to remote destination :

```plaintext
ssfcp[.exe] [-b bounce_file] [-c config_file] [-p port] path/to/file host@absolute/path/directory_destination
```

```plaintext
ssfcp[.exe] [-b bounce_file] [-c config_file] [-p port] path/to/file* host@absolute/path/directory_destination
```

#### From stdin to remote destination

```plaintext
data_in_stdin | ssfcp[.exe] [-b bounce_file] [-c config_file] [-p port] -t host@path/to/destination/file_destination
```

#### Copy remote files to local destination :

```plaintext
ssfcp[.exe] [-b bounce_file] [-c config_file] [-p port] remote_host@path/to/file absolute/path/directory_destination
```

```plaintext
ssfcp[.exe] [-b bounce_file] [-c config_file] [-p port] remote_host@path/to/file* absolute/path/directory_destination
```

### File example

#### Bounce file (relay servers)

```plaintext
127.0.0.1:8002
127.0.0.1:8003
```

#### Configuration file

```plaintext
{
  "ssf": {
    "tls" : {
      "ca_cert_path": "./certs/trusted/ca.crt",
      "cert_path": "./certs/certificate.crt",
      "key_path": "./certs/private.key",
      "key_password": "",
      "dh_path": "./certs/dh4096.pem",
      "cipher_alg": "DHE-RSA-AES256-GCM-SHA384"
    },
    "http_proxy" : {
      "host": "",
      "port": "",
      "credentials": {
        "username": "",
        "password": "",
        "domain": "",
        "reuse_ntlm": "true",
        "reuse_nego": "true"
      }
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
      "file_copy": { "enable": false },
      "shell": {
        "enable": false,
        "path": "/bin/bash|C:\\windows\\system32\\cmd.exe",
        "args": ""
      },
      "socks": { "enable": true }
    }
  }
}
```

* _tls.ca_cert_path_      : relative or absolute path to the CA certificate file
* _tls.cert_path_         : relative or absolute path to the instance certificate file
* _tls.key_path_          : relative or absolute path to the private key file
* _tls.dh_path_           : relative or absolute path to the Diffie-Hellman file
* _tls.cipher_alg_        : cipher algorithm
* _http_proxy.host_                   : HTTP proxy host
* _http_proxy.port_                   : HTTP proxy port
* _http_proxy.credentials.username_   : proxy username credentials (all platform: Basic or Digest, Windows: NTLM and Negotiate if reuse = false)
* _http_proxy.credentials.password_   : proxy password credentials (all platform: Basic or Digest, Windows: NTLM and Negotiate if reuse = false)
* _http_proxy.credentials.domain_     : user domain (NTLM and Negotiate auth on Windows only)
* _http_proxy.credentials.reuse_ntlm_ : reuse current computer user credentials to authenticate with proxy NTLM auth (SSO)
* _http_proxy.credentials.reuse_kerb_ : reuse current computer user credentials (Kerberos ticket) to authenticate with proxy Negotiate auth (SSO)
* _services.*.enable_   : [enable/disable microservice](#microservices)
* _services.*.gateway_ports_ : enable/disable gateway ports
* _services.shell.path_ : binary path used for shell creation (optional)
* _services.shell.args_ : binary arguments used for shell creation (optional)

## How to configure

### Generating certificates for TLS connections

#### With tool script

```bash
./tools/generate_cert.sh /path/to/store/certs
```

The first argument should be the directory where the CA and certificates will be generated

#### Manually

##### Generating Diffie-Hellman parameters

```bash
openssl dhparam 4096 -outform PEM -out dh4096.pem
```

##### Generating a self-signed Certification Authority (CA)
First of all, create a file named *extfile.txt* containing the following lines:

```plaintext
[ v3_req_p ]
basicConstraints = CA:FALSE
keyUsage = nonRepudiation, digitalSignature, keyEncipherment
```

Then, generate a self-signed certificate (the CA) *ca.crt* and its private key *ca.key*:

```bash
openssl req -x509 -nodes -newkey rsa:4096 -keyout ca.key -out ca.crt -days 3650
```

##### Generating a certificate (signed with the CA) and its private key

Generate a private key *private.key* and signing request *certificate.csr*:

```bash
openssl req -newkey rsa:4096 -nodes -keyout private.key -out certificate.csr
```

Sign with the CA (*ca.crt*, *ca.key*) the signing request to get the certificate *certificate.pem* :

```bash
openssl x509 -extfile extfile.txt -extensions v3_req_p -req -sha1 -days 3650 -CA ca.crt -CAkey ca.key -CAcreateserial -in certificate.csr -out certificate.pem
```

### Configuration file

#### TLS

With default options, the following files and folders should be in the directory of execution of a client or a server:

* `./certs/dh4096.pem`
* `./certs/certificate.crt`
* `./certs/private.key`
* `./certs/trusted/ca.crt`

Where:

* *dh4096.pem* contains the Diffie-Hellman parameters (see above for how to generate the file)
* *certificate.crt* and *private.key* are the certificate and the private key of the ssf server or client
* *ca.crt* is the concatenated list of certificates trusted by the ssf server or client

However, if you want those files at different paths, it is possible to customize them with the configuration file option *-c*.

An example is given in the file example section.

#### Microservices

SSF is using microservices to build its features (TCP forwarding, remote SOCKS, ...)

There are 7 microservices:
* stream_forwarder
* stream_listener
* datagram_forwarder
* datagram_listener
* file_copy
* socks
* shell

Each feature is the combination of at least one client side microservice and one server side microservice.

This table sums up how each feature is assembled:

| ssfc feature                | microservice client side | microservice server side |
|:----------------------------|:-------------------------|:-------------------------|
| `-L`: TCP forwarding        | stream_listener          | stream_forwarder         |
| `-R`: remote TCP forwarding | stream_forwarder         | stream_listener          |
| `-U`: UDP forwarding        | datagram_listener        | datagram_forwarder       |
| `-V`: remote UDP forwarding | datagram_forwarder       | datagram_listener        |
| `-D`: SOCKS                 | stream_listener          | socks                    |
| `-F`: remote SOCKS          | socks                    | stream_listener          |
| `-X`: shell                 | stream_listener          | shell                    |
| `-Y`: remote shell          | shell                    | stream_listener          |

This architecture makes it easier to build remote features: they use the same microservices but on the opposite side.

`ssfc` and `ssfs` come with pre-enabled microservices.
Here is the default microservices configuration:

```
"ssf": {
  "services": {
    "datagram_forwarder": { "enable": true },
    "datagram_listener": { "enable": true },
    "stream_forwarder": { "enable": true },
    "stream_listener": { "enable": true },
    "socks": { "enable": true },
    "file_copy": { "enable": false },
    "shell": { "enable": false }
  }
}
```

To enable or disable a microservice, set its `enable` option to `true` or `false`.

Trying to use a feature requiring a disabled microservice will result in an error message.

### Relay chain file

This file will contain the bounce servers and ports which will be used to establish the connection.
They will be listed as follow :

```plaintext
SERVER1:PORT1
SERVER2:PORT2
SERVER3:PORT3
```

The chain will be CLIENT -> SERVER1:PORT1 -> SERVER2:PORT2 -> SERVER3:PORT3 -> TARGET

## How to build

### Requirements

  * Winrar >= 5.2.1 (Third party builds on windows)
  * Boost >= 1.61.0
  * OpenSSL >= 1.0.2
  * Google Test = 1.7.0
  * CMake >= 2.8.11
  * nasm (openssl build on windows)
  * Perl | Active Perl >= 5.20 (openssl build on windows)
  * C++11 compiler (Visual Studio 2013, Clang, g++, etc.)
  * libkrb5-dev or equivalent (gssapi on linux)

SSF_SECURITY:

* **STANDARD**: the project will be build with standard security features
* **FORCE_TCP_ONLY**: the project will be built without security features to facilitate debugging

### Build SSF on Windows

* Go in project directory

```bash
cd PROJECT_PATH
```

* Copy [Boost archive](http://www.boost.org/users/download/) in ``third_party/boost``

```bash
cp boost_1_XX_Y.tar.bz2 PROJECT_PATH/third_party/boost
```

* Copy [OpenSSL archive](https://www.openssl.org/source/) in ``third_party/openssl``

```bash
cp openssl-1.0.XY.tar.gz PROJECT_PATH/third_party/openssl
```

If you are using *openssl-1.0.2a*, you need to fix the file ``crypto/x509v3/v3_scts.c``. It contains an incorrect ``#include`` line.
Copy [the diff from OpenSSL Github](https://github.com/openssl/openssl/commit/77b1f87214224689a84db21d2eb54e9497186d93.diff)
(ignore the 2 first lines) and put it in ``PROJECT_PATH/third_party/openssl/patches``. The build script will then patch the sources.

* Copy [GTest archive](https://github.com/google/googletest/archive/release-1.7.0.zip) in ``third_party/gtest``

 ```bash
 cp gtest-1.X.Y.zip PROJECT_PATH/third_party/gtest
 ```

* Generate project

```bash
git submodule update --init --recursive
mkdir PROJECT_PATH/build
cd PROJECT_PATH/build
cmake -DSSF_SECURITY:STRING="STANDARD|FORCE_TCP_ONLY" ../
```

* Build project

```bash
cd PROJECT_PATH/build
cmake --build . --config Debug|Release
```

### Build SSF on Linux

* Go in project directory

```bash
cd PROJECT_PATH
```

* Copy [Boost archive](http://www.boost.org/users/download/) in ``third_party/boost``

```bash
cp boost_1_XX_Y.tar.bz2 PROJECT_PATH/third_party/boost
```

* Copy [OpenSSL archive](https://www.openssl.org/source/) in ``third_party/openssl``

```bash
cp openssl-1.0.XY.tar.gz PROJECT_PATH/third_party/openssl
```

* Copy [GTest archive](https://github.com/google/googletest/archive/release-1.7.0.zip) in ``third_party/gtest``

```bash
cp gtest-1.X.Y.zip PROJECT_PATH/third_party/gtest
```

* Generate project

```bash
git submodule update --init --recursive
mkdir PROJECT_PATH/build
cd PROJECT_PATH/build
cmake -DCMAKE_BUILD_TYPE=Release|Debug -DSSF_SECURITY:STRING="STANDARD|FORCE_TCP_ONLY" ../
```

* Build project

```bash
cd PROJECT_PATH/build
cmake --build . -- -j
```

### Build SSF on Mac OS X

* Go in project directory

```bash
cd PROJECT_PATH
```

* Copy [Boost archive](http://www.boost.org/users/download/) in ``third_party/boost``

```bash
cp boost_1_XX_Y.tar.bz2 PROJECT_PATH/third_party/boost
```

* Copy [OpenSSL archive](https://www.openssl.org/source/) in ``third_party/openssl``

```bash
cp openssl-1.0.XY.tar.gz PROJECT_PATH/third_party/openssl
```

* Copy [GTest archive](https://github.com/google/googletest/archive/release-1.7.0.zip) in ``third_party/gtest``

```bash
cp gtest-1.X.Y.zip PROJECT_PATH/third_party/gtest
```

* Generate project

```bash
git submodule update --init --recursive
mkdir PROJECT_PATH/build
cd PROJECT_PATH/build
cmake -DCMAKE_BUILD_TYPE=Release|Debug -DSSF_SECURITY:STRING="STANDARD|FORCE_TCP_ONLY" ../
```

* Build project

```bash
cd PROJECT_PATH/build
cmake --build .
```
