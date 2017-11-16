#ifndef SSF_SERVICES_OPTION_PARSER_H_
#define SSF_SERVICES_OPTION_PARSER_H_

#include <cstdint>

#include <string>

#include <boost/system/error_code.hpp>

namespace ssf {
namespace services {

struct Endpoint {
  Endpoint();

  std::string addr;
  uint16_t port;
};

struct ForwardOptions {
  ForwardOptions();
  Endpoint from;
  Endpoint to;
};

class OptionParser {
 public:
  // Parse forward option
  // Expected format: [[loc_addr]:]loc_port:rem_addr:rem_port
  // If loc_addr is empty on purpose, set its value as "*"
  // Set ec if parsing failed
  static ForwardOptions ParseForwardOptions(const std::string& option,
                                            boost::system::error_code& ec);

  // Parse listening option
  // Expected format: [[loc_addr]:]loc_port
  // If loc_addr is empty on purpose, set its value as "*"
  // Set ec if parsing failed
  static Endpoint ParseListeningOption(const std::string& option,
                                       boost::system::error_code& ec);

  static uint16_t ParsePort(const std::string& port,
                            boost::system::error_code& ec);
};

}  // services
}  // ssf

#endif  // SSF_SERVICES_OPTION_PARSER_H_