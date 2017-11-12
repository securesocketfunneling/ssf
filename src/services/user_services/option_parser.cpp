#include <ssf/log/log.h>

#include "common/error/error.h"

#include "services/user_services/option_parser.h"

namespace ssf {
namespace services {

Endpoint::Endpoint() : addr(""), port(0) {}

ForwardOptions::ForwardOptions() : from(), to() {}

ForwardOptions OptionParser::ParseForwardOptions(
    const std::string& option, boost::system::error_code& ec) {
  ForwardOptions forward_options;
  const char* p;
  char* end;
  unsigned long val;
  std::string::size_type index, e;

  index = option.rfind(':');
  if (index == std::string::npos) {
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return {};
  }

  end = NULL;
  p = option.c_str() + index + 1;
  val = std::strtoul(p, &end, 10);
  if (p == end || *end != '\0' || val > 65535) {
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return {};
  }
  forward_options.to.port = static_cast<uint16_t>(val);

  e = index;
  index = option.rfind(':', e - 1);
  if (index == std::string::npos) {
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return {};
  }
  forward_options.to.addr = std::string(option, index + 1, e - (index + 1));

  e = index;
  index = option.rfind(':', e - 1);

  end = NULL;
  if (index != std::string::npos) {
    auto s = std::string(option, 0, index);
    forward_options.from.addr = (s == "") ? "*" : s;
    p = option.c_str() + index + 1;
  } else {
    forward_options.from.addr = "";
    p = option.c_str();
  }
  val = std::strtoul(p, &end, 10);
  if (p == end || *end != ':' || val > 65535) {
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return {};
  }
  forward_options.from.port = static_cast<uint16_t>(val);

  return forward_options;
}

Endpoint OptionParser::ParseListeningOption(const std::string& option,
                                            boost::system::error_code& ec) {
  Endpoint listening_option;
  unsigned long val;
  const char* p;
  char* end = NULL;
  std::string::size_type index = option.find(':');

  if (index != std::string::npos) {
    auto s = std::string(option, 0, index);
    listening_option.addr = (s == "") ? "*" : s;
    p = option.c_str() + index + 1;
  } else {
    listening_option.addr = "";
    p = option.c_str();
  }
  val = std::strtoul(p, &end, 10);
  if (p == end || *end != '\0' || val > 65535) {
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return {};
  }
  listening_option.port = static_cast<uint16_t>(val);

  return listening_option;
}

uint16_t OptionParser::ParsePort(const std::string& port,
                                 boost::system::error_code& ec) {
  unsigned long val;
  char* end = NULL;

  val = std::strtoul(port.c_str(), &end, 10);
  if (*end != '\0' || val > 65535) {
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return 0;
  }

  return static_cast<uint16_t>(val);
}

}  // services
}  // ssf
