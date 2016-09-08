#include "core/command_line/standard/command_line.h"

#include "common/error/error.h"

namespace ssf {
namespace command_line {
namespace standard {

CommandLine::CommandLine(bool is_server)
    : BaseCommandLine(),
      is_server_(is_server),
      show_status_(false),
      relay_only_(false),
      gateway_ports_(false) {}

void CommandLine::PopulateBasicOptions(OptionDescription& basic_opts) {}

void CommandLine::PopulateLocalOptions(OptionDescription& local_opts) {
  auto* p_host_option =
      boost::program_options::value<std::string>(&host_)->value_name("host");
  if (!IsServerCli()) {
    p_host_option->required();
  }

  if (IsServerCli()) {
    local_opts.add_options()(
        "relay-only,R",
        boost::program_options::bool_switch()->default_value(false),
        "Server will only relay connections");
  }

  // clang-format off
  local_opts.add_options()
      ("host,H",
         p_host_option,
         "Set host");
  local_opts.add_options()
      ("gateway-ports,g",
         boost::program_options::bool_switch()->default_value(false),
         "Allow gateway ports. At connection, client will be allowed to "
         "specify listening network interface on every services");
  local_opts.add_options()
      ("status,S",
         boost::program_options::bool_switch()->default_value(false),
         "Display microservices status (on/off)");
  // clang-format on
}

void CommandLine::PopulatePositionalOptions(PosOptionDescription& pos_opts) {
  pos_opts.add("host", 1);
}

void CommandLine::PopulateCommandLine(OptionDescription& command_line) {}

bool CommandLine::IsServerCli() { return is_server_; }

void CommandLine::ParseOptions(const VariableMap& vm,
                               ParsedParameters& parsed_params,
                               boost::system::error_code& ec) {
  if (vm.count("host")) {
    host_ = vm["host"].as<std::string>();
    host_set_ = true;
  }
  if (vm.count("status")) {
    show_status_ = vm["status"].as<bool>();
  }
  if (vm.count("relay-only")) {
    relay_only_ = vm["relay-only"].as<bool>();
  }
  if (vm.count("gateway-ports")) {
    gateway_ports_ = vm["gateway-ports"].as<bool>();
  }
}

std::string CommandLine::GetUsageDesc() {
  std::stringstream ss_desc;
  ss_desc << exec_name_ << " [options] [host]";
  return ss_desc.str();
}

}  // standard
}  // command_line
}  // ssf
