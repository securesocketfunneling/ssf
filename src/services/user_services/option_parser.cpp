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
  char *end;
  unsigned long val;
  std::string::size_type s, e;

  s = option.rfind(':');
  if (s == std::string::npos) {
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return {};
  }

  end = NULL;
  val = std::strtoul(option.c_str() + s + 1, &end, 10);
  if (*end != '\0' || val > 65535) {
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return {};
  }
  forward_options.to.port = val;

  e = s;
  s = option.rfind(':', e - 1);
  if (s == std::string::npos) {
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return {};
  }
  forward_options.to.addr = std::string(option, s + 1, e - (s + 1));

  e = s;
  s = option.rfind(':', e - 1);

  end = NULL;
  if (s != std::string::npos) {
    forward_options.from.addr = std::string(option, 0, s);
    val = std::strtoul(option.c_str() + s + 1, &end, 10);
  } else {
    forward_options.from.addr = "";
    val = std::strtoul(option.c_str(), &end, 10);
  }
  if (*end != ':' || val > 65535) {
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return {};
  }
  forward_options.from.port = val;

  return forward_options;
}

Endpoint OptionParser::ParseListeningOption(const std::string& option,
                                            boost::system::error_code& ec) {
  Endpoint listening_option;
  unsigned long val;
  char *end = NULL;
  std::string::size_type s = option.find(':');

  if (s != std::string::npos) {
    listening_option.addr = std::string(option, 0, s);
    val = std::strtoul(option.c_str() + s + 1, &end, 10);
  } else {
    listening_option.addr = "";
    val = std::strtoul(option.c_str(), &end, 10);
  }
  if (*end != '\0' || val > 65535) {
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return {};
  }
  listening_option.port = val;

  return listening_option;
}

uint16_t OptionParser::ParsePort(const std::string& port,
                                 boost::system::error_code& ec) {
  unsigned long val;
  char *end = NULL;

  val = std::strtoul(port.c_str(), &end, 10);
  if (*end != '\0' || val > 65535) {
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return 0;
  }

  return val;
}


}  // services
}  // ssf
