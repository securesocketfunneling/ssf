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

```plaintext
ssf<c|s>[.exe] [-h] [-v verb_level] [-q] [-L loc:ip:dest] [-R rem:ip:dest] [-D port] [-F port] [-U loc:ip:dest] [-V rem:ip:dest] [-X port] [-Y port] [-b bounce_file] [-c config_file] [-p port] [host]
```

* -v : Verbosity level (critical, error, warning, info, debug, trace), default is info
* -q : Quiet mode (no log)
* -L : TCP port forwarding with *loc* as the local TCP port, *ip* and *dest* as destination toward which the forward should be done from the server.
* -R : TCP remote port forwarding with *rem* as the TCP port to forward from the remote host, *ip* and *dest* as destination toward which the forward should be done from the client.
* -D : open a port (*port*) on the client to connect to a SOCKS server on the server from the client.
* -F : open a port (*port*) on the server to connect to a SOCKS server on the client from the server.
* -U : UDP port forwarding with *loc* as the UDP port to forward from the client, *ip* and *dest* as destination toward which the forward should be done from the server.
* -V : UDP remote port forwarding with *rem* as the UDP port to forward from the server, *ip* and *dest* as destination toward which the forward should be done from the client.
* -X : open a port (*port*) on the client side, each connection to that port creates a process with I/O forwarded to/from the server side (the binary used can be set with the config file)
* -Y : open a port (*port*) on the server side, each connection to that port creates a process with I/O forwarded to/from the client side (the binary used can be set with the config file)
* -b : *bounce_file* is the file containing the list of relays to use.
* -c : *config_file* is the config file containing configuration for SSF (TLS configuration).
* -p : *port* is the port on which to listen (for the server) or to connect (for the client). The default value is 8011.
* host : the IP address or the name of the remote server to connect to.

#### Server example

Server will listen on all network interfaces on port **8011**

```plaintext
ssfs[.exe]
```

Server will listen on **192.168.0.1:9000**

```plaintext
ssfs[.exe] -p 9000 192.168.0.1
```

#### Client example

Client will open port 9000 locally and wait SOCKS requests to be transferred to
server **192.168.0.1:8000**

```plaintext
ssfc[.exe] -D 9000 -b bounce.txt -c config.json -p 8000 192.168.0.1
```

### Copy command line

```plaintext
ssfcp[.exe] [-h] [-b bounce_file] [-c config_file] [-p port] [-t] [host@]path [[host@]path]
```

* -b : *bounce_file* is the file containing the list of relays to use.
* -c : *config_file* is the config file containing configuration for SSF (TLS configuration).
* -t : input from stdin

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

#### Config file

```plaintext
{
    "ssf": {
        "tls": {
            "ca_cert_path": "./certs/trusted/ca.crt",
            "cert_path": "./certs/certificate.crt",
            "key_path": "./certs/private.key",
            "dh_path": "./certs/dh4096.pem",
            "cipher_alg": "DHE-RSA-AES256-GCM-SHA384"
        },
        "http_proxy": {
            "host": "proxy.example.com",
            "port": "3128",
            "credentials": {
                "username": "user",
                "password": "password",
                "domain": "EXAMPLE.COM",
                "reuse_ntlm": "true",
                "reuse_kerb": "true"
            }
        },
        "services": {
            "shell": {
                "path": "/bin/bash",
                "args": ""
            }
        }
    }
}
```

* *tls.ca_cert_path*      : relative or absolute path to the CA certificate file
* *tls.cert_path*         : relative or absolute path to the instance certificate file
* *tls.key_path*          : relative or absolute path to the private key file
* *tls.dh_path*           : relative or absolute path to the Diffie-Hellman file
* *tls.cipher_alg*        : cipher algorithm
* *http_proxy.host*                   : HTTP proxy host
* *http_proxy.port*                   : HTTP proxy port
* *http_proxy.credentials.username*   : proxy username credentials (all platform: Basic or Digest, Windows: NTLM and Negotiate if reuse = false)
* *http_proxy.credentials.password*   : proxy password credentials (all platform: Basic or Digest, Windows: NTLM and Negotiate if reuse = false)
* *http_proxy.credentials.domain*     : user domain (NTLM and Negotiate auth on Windows only)
* *http_proxy.credentials.reuse_ntlm* : reuse current computer user credentials to authenticate with proxy NTLM auth (SSO)
* *http_proxy.credentials.reuse_kerb* : reuse current computer user credentials (Kerberos ticket) to authenticate with proxy Negotiate auth (SSO)
* *services.shell.path* : binary path used for shell creation (optional)
* *services.shell.args* : binary arguments used for shell creation (optional)

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

With default options, the following files and folders should be in the directory of execution of a client or a server:

* ./certs/dh4096.pem
* ./certs/certificate.crt
* ./certs/private.key
* ./certs/trusted/ca.crt

Where:

* *dh4096.pem* contains the Diffie-Hellman parameters (see above for how to generate the file)
* *certificate.crt* and *private.key* are the certificate and the private key of the ssf server or client
* *ca.crt* is the concatenated list of certificates trusted by the ssf server or client

However, if you want those files at different paths, it is possible to customize them with the configuration file option *-c*.

An example is given in the file example section.

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
