# Change Log

## 2.1.0

Features:
* TLS layer over circuit layer
* HTTP proxy support (CONNECT method), cf. configuration file
* HTTP proxy authentication support (Basic, Digest, NTLM [windows only], Negotiate), cf. configuration file
* Basic shell through socket (-X and -Y options)
* Server network interface option

Fixed bugs:
* Linux static link to libstdc++
* Linux dependency to GLIBC2.14 (memcpy)
* Stop behavior (signal instead of user input)
* Port forwarding listening side on localhost only

## 2.0.0

/!\ BC break with version 1.\*.\*

Features:
* New network layer based on SSF network framework

## 1.1.0
Features:
* ssfcp: file copy between client and server
* Rename executables:
  * SSF_Client -> ssfc
  * SSF_Server -> ssfs

Fixed bugs:
* Crash issue due to exception when resolving endpoint
* Exception safety
* Windows compilation warnings (64bits)

## 1.0.0
Features:
* Local and remote TCP port forwarding (-L and -R options)
* Local and remote UDP port forwarding (-U and -V options)
* Local and remote SOCKS server (-D and -F options)
* Native relay protocol (-b option)
* Multi platform (Windows, Linux and OSX)
* TLS connection with the strongest cipher-suites
* Standalone executables
