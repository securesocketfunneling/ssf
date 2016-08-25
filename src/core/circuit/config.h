#ifndef SSF_CORE_CIRCUIT_CONFIG_H_
#define SSF_CORE_CIRCUIT_CONFIG_H_

#include <list>
#include <string>

#include <boost/system/error_code.hpp>

namespace ssf {
namespace circuit {

class CircuitNode {
 public:
  CircuitNode(const std::string& addr, const std::string& port);

 public:
  inline std::string addr() const { return addr_; }
  inline void set_addr(const std::string& addr) { addr_ = addr; }

  inline std::string port() const { return port_; }
  inline void set_port(const std::string& port) { port_ = port; }

 private:
  std::string addr_;
  std::string port_;
};

using NodeList = std::list<CircuitNode>;

class Config {
 public:
 public:
  Config();

 public:
  /**
   * Parse circuit file
   * If no file provided, try to load circuit data "circuit.txt" file
   * @param filepath
   */
  void Update(const std::string& filepath, boost::system::error_code& ec);

  /**
   * Log configuration
   */
  void Log() const;

  inline NodeList nodes() const { return nodes_; };

 private:
  NodeList nodes_;
};

}  // circuit
}  // ssf

#endif  // SSF_CORE_CIRCUIT_CONFIG_H_
