#include "core/command_line/copy/command_line.h"

#include <cstdint>

#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <regex>
#include <memory>

#include <boost/program_options.hpp>
#include <boost/system/error_code.hpp>

#include "common/error/error.h"

#include "versions.h"

namespace ssf {
namespace command_line {
namespace copy {

CommandLine::CommandLine() : BaseCommandLine(), from_stdin_(false) {}

void CommandLine::PopulateBasicOptions(OptionDescription& basic_opts) {}

void CommandLine::PopulateLocalOptions(OptionDescription& local_opts) {}

void CommandLine::PopulatePositionalOptions(PosOptionDescription& pos_opts) {
  pos_opts.add("arg1", 1);
  pos_opts.add("arg2", 1);
}

void CommandLine::PopulateCommandLine(OptionDescription& command_line) {
  // clang-format off
  boost::program_options::options_description copy_options("Copy options");
  copy_options.add_options()
    ("stdin,t", boost::program_options::bool_switch()->default_value(false), "Input will be stdin")
    ("arg1",
        boost::program_options::value<std::string>()->required(),
        "[host@]/absolute/path/file if host is present, " \
        "the file will be copied from the remote host to local")
    ("arg2",
        boost::program_options::value<std::string>(),
        "[host@]/absolute/path/file if host is present, " \
        "the file will be copied from local to host");
  // clang-format on

  command_line.add(copy_options);
}

bool CommandLine::IsServerCli() { return false; }

bool CommandLine::from_stdin() const { return from_stdin_; }

bool CommandLine::from_local_to_remote() const { return from_local_to_remote_; }

std::string CommandLine::input_pattern() const { return input_pattern_; }

std::string CommandLine::output_pattern() const { return output_pattern_; }

void CommandLine::ParseOptions(const VariableMap& vm,
                               ParsedParameters& parsed_params,
                               boost::system::error_code& ec) {
  const auto& stdin_it = vm.find("stdin");
  const auto& vm_end_it = vm.end();

  if (stdin_it != vm_end_it) {
    from_stdin_ = stdin_it->second.as<bool>();
  }

  ParseFirstArgument(vm["arg1"].as<std::string>(), ec);

  const auto& arg2_it = vm.find("arg2");
  if (arg2_it != vm_end_it) {
    ParseSecondArgument(arg2_it->second.as<std::string>(), ec);
  }
}

void CommandLine::ParseFirstArgument(const std::string& first_arg,
                                     boost::system::error_code& parse_ec) {
  if (from_stdin_) {
    // expecting host:filepath syntax
    ExtractHostPattern(first_arg, &host_, &output_pattern_, parse_ec);
    if (!parse_ec) {
      host_set_ = true;
      from_local_to_remote_ = true;
    }
  } else {
    // expecting host:dirpath or filepath syntax
    boost::system::error_code extract_ec;
    ExtractHostPattern(first_arg, &host_, &input_pattern_, extract_ec);
    if (!extract_ec) {
      host_set_ = true;
      from_local_to_remote_ = false;
    } else {
      // not host:dirpath syntax so it is filepath syntax
      input_pattern_ = first_arg;
      from_local_to_remote_ = true;
    }
  }
}

void CommandLine::ParseSecondArgument(const std::string& second_arg,
                                      boost::system::error_code& parse_ec) {
  if (from_stdin_) {
    // no second arg should be provided
    SSF_LOG(kLogError)
        << "command line: parsing failed: two args provided with stdin option";
    parse_ec.assign(::error::invalid_argument, ::error::get_ssf_category());

    return;
  }

  // expecting host:filepath or filepath syntax
  if (from_local_to_remote_) {
    // expecting host:dirpath
    ExtractHostPattern(second_arg, &host_, &output_pattern_, parse_ec);
    if (parse_ec) {
      host_set_ = false;
      return;
    }

    host_set_ = true;
  } else {
    // expecting dirpath
    output_pattern_ = second_arg;
  }

  // Insert trailing slash if not present
  auto last_slash_pos = output_pattern_.find_last_of('/');
  if (last_slash_pos != output_pattern_.size() - 1) {
    output_pattern_ += '/';
  }
}

void CommandLine::ExtractHostPattern(const std::string& string,
                                     std::string* p_host,
                                     std::string* p_pattern,
                                     boost::system::error_code& ec) const {
  std::size_t found = string.find_first_of(GetHostDirectorySeparator());
  if (found == std::string::npos || string.empty()) {
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return;
  }

  *p_host = string.substr(0, found);
  *p_pattern = string.substr(found + 1);
  ec.assign(::error::success, ::error::get_ssf_category());
}

char CommandLine::GetHostDirectorySeparator() const { return '@'; }

}  // copy
}  // command_line
}  // ssf
