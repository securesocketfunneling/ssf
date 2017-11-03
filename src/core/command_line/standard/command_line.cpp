#include <sstream>

#include "core/command_line/standard/command_line.h"

#include "common/error/error.h"

namespace ssf {
namespace command_line {

StandardCommandLine::StandardCommandLine(bool is_server)
    : Base(),
      is_server_(is_server),
      show_status_(false),
      relay_only_(false),
      gateway_ports_(false),
      max_connection_attempts_(1),
      reconnection_timeout_(60) {}

void StandardCommandLine::InitOptions(Options& opts) {
  Base::InitOptions(opts);

  if (IsServerCli()) {
    // server cli
    opts.add_options()
      ("R,relay-only", "The server will only relay connections")
      ("l,bind-address", "Server bind address", cxxopts::value<std::string>());
  } else {
    // client cli
    opts.add_options()
      ("m,max-connect-attempts",
       "Number of unsuccessful connection attempts before stopping",
       cxxopts::value<uint32_t>()->default_value("1"))
      ("t,reconnect-delay",
       "Time to wait before attempting to reconnect",
        cxxopts::value<uint32_t>()->default_value("60"))
      ("n,no-reconnect",
       "Do not attempt to reconnect after loosing a connection")
      ("server-address", "", cxxopts::value<std::vector<std::string>>());

    opts.parse_positional("server-address");

    opts.positional_help("server_address");
  }

  opts.add_options()
    ("g,gateway-ports", "Enable gateway ports")
    ("S,status", "Display microservices status");
}

bool StandardCommandLine::IsServerCli() { return is_server_; }

void StandardCommandLine::ParseOptions(const Options& opts,
                                       boost::system::error_code& ec) {
  if (IsServerCli()) {
    try {
      host_ = opts["bind-address"].as<std::string>();
    } catch (cxxopts::option_not_present_exception) {
      host_ = "";
    }
  } else {
    auto& server_address = opts["server-address"].as<std::vector<std::string>>();

    if (server_address.size() > 1) {
      SSF_LOG("cli", error, "too many arguments");
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    } else if (!server_address.size()) {
      SSF_LOG("cli", error, "missing arguments");
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    } else
      host_ = server_address[0];
  }

  show_status_ = opts.count("status");

  if (IsServerCli()) {
    relay_only_ = opts.count("relay-only");
  } else {
    uint32_t max_attempts = opts["max-connect-attempts"].as<uint32_t>();
    if (max_attempts == 0) {
      SSF_LOG("cli", error, "option max-connect-attempts must be > 0");
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    } else {
      max_connection_attempts_ = max_attempts;
    }

    reconnection_timeout_ = opts["reconnect-delay"].as<uint32_t>();
    no_reconnection_ = opts.count("no-reconnect");
  }

  gateway_ports_ = opts.count("gateway-ports");
}

}  // command_line
}  // ssf
