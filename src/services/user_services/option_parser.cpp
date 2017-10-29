#ifndef BOOST_SPIRIT_USE_PHOENIX_V3
#define BOOST_SPIRIT_USE_PHOENIX_V3 1
#endif
#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/optional.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/classic.hpp>

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "services/user_services/option_parser.h"

namespace ssf {
namespace services {

Endpoint::Endpoint() : addr(""), port(0) {}

ForwardOptions::ForwardOptions() : from(), to() {}

ForwardOptions OptionParser::ParseForwardOptions(
    const std::string& option, boost::system::error_code& ec) {
  using boost::spirit::qi::ushort_;
  using boost::spirit::qi::alnum;
  using boost::spirit::qi::char_;
  using boost::spirit::qi::lit;
  using boost::spirit::qi::rule;
  typedef std::string::const_iterator str_it;

  ForwardOptions forward_options;

  rule<str_it, std::string()> local_addr_pattern = *char_("0-9a-zA-Z.-");
  rule<str_it, std::string()> target_pattern = +char_("0-9a-zA-Z.-");

  str_it begin = option.begin(), end = option.end();

  // basic format
  if (boost::spirit::qi::parse(
          begin, end, ushort_ >> ":" >> target_pattern >> ":" >> ushort_,
          forward_options.from.port, forward_options.to.addr,
          forward_options.to.port) &&
      begin == end) {
    ec.assign(::error::success, ::error::get_ssf_category());
    return forward_options;
  }

  forward_options = {};
  begin = option.begin(), end = option.end();
  // extended format
  if (boost::spirit::qi::parse(
          begin, end, local_addr_pattern >> ":" >> ushort_ >> ":" >>
                          target_pattern >> ":" >> ushort_,
          forward_options.from.addr, forward_options.from.port,
          forward_options.to.addr, forward_options.to.port) &&
      begin == end) {
    if (forward_options.from.addr.empty()) {
      // empty value means all network interfaces
      forward_options.from.addr = "*";
    }

    // not found
    ec.assign(::error::success, ::error::get_ssf_category());
    return forward_options;
  }

  ec.assign(::error::invalid_argument, ::error::get_ssf_category());

  return forward_options;
}

Endpoint OptionParser::ParseListeningOption(const std::string& option,
                                            boost::system::error_code& ec) {
  using boost::spirit::qi::ushort_;
  using boost::spirit::qi::alnum;
  using boost::spirit::qi::char_;
  using boost::spirit::qi::lit;
  using boost::spirit::qi::rule;
  typedef std::string::const_iterator str_it;

  Endpoint listening_option;

  rule<str_it, std::string()> local_addr_pattern = *char_("0-9a-zA-Z.-");

  str_it begin = option.begin(), end = option.end();

  // basic format
  if (boost::spirit::qi::parse(begin, end, ushort_, listening_option.port) &&
      begin == end) {
    listening_option.addr = "";
    ec.assign(::error::success, ::error::get_ssf_category());
    return listening_option;
  }

  listening_option = {};
  begin = option.begin(), end = option.end();
  // extended format
  if (boost::spirit::qi::parse(begin, end, local_addr_pattern >> ":" >> ushort_,
                               listening_option.addr, listening_option.port) &&
      begin == end) {
    if (listening_option.addr.empty()) {
      // empty value means all network interfaces
      listening_option.addr = "*";
    }
    ec.assign(::error::success, ::error::get_ssf_category());
    return listening_option;
  }

  // not found
  ec.assign(::error::invalid_argument, ::error::get_ssf_category());

  return listening_option;
}

uint16_t OptionParser::ParsePort(const std::string& port,
                                 boost::system::error_code& ec) {
  try {
    std::size_t pos = 0;
    uint32_t res = std::stoul(port, &pos);
    if (pos != port.size()) {
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      return 0;
    }
    if (res < 1 || res > 65535) {
      SSF_LOG("parser", error, "invalid port: {} not in [1 - 65355]", port);
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      return 0;
    }
    return res;
  } catch (...) {
    SSF_LOG("parser", error, "invalid port: {}", port);
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return 0;
  }
}

}  // services
}  // ssf
