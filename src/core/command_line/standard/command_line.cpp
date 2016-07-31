#include "core/command_line/standard/command_line.h"

#include "common/error/error.h"

namespace ssf {
namespace command_line {
namespace standard {

CommandLine::CommandLine(bool is_server)
    : BaseCommandLine(), is_server_(is_server) {}

void CommandLine::PopulateBasicOptions(OptionDescription& basic_opts) {}

void CommandLine::PopulateLocalOptions(OptionDescription& local_opts) {
  // clang-format off
  local_opts.add_options()
      ("host,H",
         (IsServerCli() ?
           boost::program_options::value<std::string>(&host_) :
           boost::program_options::value<std::string>(&host_)->required()),
         "Set host");
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
}

}  // standard
}  // command_line
}  // ssf
