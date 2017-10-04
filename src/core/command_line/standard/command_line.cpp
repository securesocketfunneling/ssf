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

StandardCommandLine::~StandardCommandLine() {}

void StandardCommandLine::PopulateBasicOptions(OptionDescription& basic_opts) {}

void StandardCommandLine::PopulateLocalOptions(OptionDescription& local_opts) {
  auto* p_host_arg =
      boost::program_options::value<std::string>(&host_)->value_name("host");

  if (IsServerCli()) {
    // server cli
    local_opts.add_options()("relay-only,R",
                             boost::program_options::bool_switch()
                                 ->value_name("relay-only")
                                 ->default_value(false),
                             "Server will only relay connections");
  } else {
    // client cli
    local_opts.add_options()("max-connection-attempts,m",
                             boost::program_options::value<uint32_t>()
                                 ->value_name("max-connection-attempts")
                                 ->default_value(1),
                             "Max connection attempts before stopping client");

    local_opts.add_options()("reconnection-timeout,t",
                             boost::program_options::value<uint32_t>()
                                 ->value_name("reconnection-timeout")
                                 ->default_value(60),
                             "Timeout between connection attempts in seconds");

    local_opts.add_options()(
        "no-reconnection,n", boost::program_options::bool_switch()
                                 ->value_name("no-reconnection")
                                 ->default_value(false),
        "Do not try to reconnect client if connection is interrupted");
  }

  local_opts.add_options()("host,H", p_host_arg, "Set server host");

  local_opts.add_options()(
      "gateway-ports,g", boost::program_options::bool_switch()
                             ->value_name("gateway-ports")
                             ->default_value(false),
      "Allow gateway ports. At connection, client will be allowed to "
      "specify listening network interface on every services");

  local_opts.add_options()("status,S", boost::program_options::bool_switch()
                                           ->value_name("status")
                                           ->default_value(false),
                           "Display microservices status (on/off)");
}

void StandardCommandLine::PopulatePositionalOptions(
    PosOptionDescription& pos_opts) {
  pos_opts.add("host", 1);
}

void StandardCommandLine::PopulateCommandLine(OptionDescription& command_line) {
}

bool StandardCommandLine::IsServerCli() { return is_server_; }

void StandardCommandLine::ParseOptions(const VariableMap& vm,
                                       boost::system::error_code& ec) {
  if (vm.count("host") && !vm["host"].empty()) {
    host_ = vm["host"].as<std::string>();
  } else {
    host_ = "";
  }

  show_status_ = vm["status"].as<bool>();

  if (IsServerCli()) {
    relay_only_ = vm["relay-only"].as<bool>();
  } else {
    uint32_t max_attempts = vm["max-connection-attempts"].as<uint32_t>();
    if (max_attempts == 0) {
      SSF_LOG(kLogError)
          << "command line: option max-connection-attempts invalid";
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    } else {
      max_connection_attempts_ = max_attempts;
    }

    reconnection_timeout_ = vm["reconnection-timeout"].as<uint32_t>();
    no_reconnection_ = vm["no-reconnection"].as<bool>();
  }

  gateway_ports_ = vm["gateway-ports"].as<bool>();
}

std::string StandardCommandLine::GetUsageDesc() {
  std::stringstream ss_desc;
  ss_desc << exec_name_ << " [options] [host]";
  return ss_desc.str();
}

}  // command_line
}  // ssf
