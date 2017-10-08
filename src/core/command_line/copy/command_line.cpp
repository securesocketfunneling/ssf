#include <cstdint>

#include <iostream>
#include <memory>

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "core/command_line/copy/command_line.h"

#include "versions.h"

namespace ssf {
namespace command_line {

static const char kHostDirectorySeparator = '@';

CopyCommandLine::CopyCommandLine() : Base(), from_stdin_(false) {}

CopyCommandLine::~CopyCommandLine() {}

void CopyCommandLine::PopulateBasicOptions(OptionDescription& basic_opts) {}

void CopyCommandLine::PopulateLocalOptions(OptionDescription& local_opts) {}

void CopyCommandLine::PopulatePositionalOptions(
    PosOptionDescription& pos_opts) {
  pos_opts.add("arg1", 1);
  pos_opts.add("arg2", 1);
}

void CopyCommandLine::PopulateCommandLine(OptionDescription& command_line) {
  // clang-format off
  boost::program_options::options_description copy_options("Copy options");
  copy_options.add_options()
    ("stdin,t",
        boost::program_options::bool_switch()->default_value(false),
        "Input will be stdin")
    ("arg1",
        boost::program_options::value<std::string>()
          ->value_name("[host@]/absolute/path/file"),
        "[host@]/absolute/path/file if host is present, " \
        "the file will be copied from remote host to local")
    ("arg2",
        boost::program_options::value<std::string>()
          ->value_name("[host@]/absolute/path/file"),
        "[host@]/absolute/path/file if host is present, " \
        "the file will be copied from local to remote host");
  // clang-format on

  command_line.add(copy_options);
}

bool CopyCommandLine::IsServerCli() { return false; }

bool CopyCommandLine::from_stdin() const { return from_stdin_; }

bool CopyCommandLine::from_local_to_remote() const {
  return from_local_to_remote_;
}

std::string CopyCommandLine::input_pattern() const { return input_pattern_; }

std::string CopyCommandLine::output_pattern() const { return output_pattern_; }

void CopyCommandLine::ParseOptions(const VariableMap& vm,
                                   boost::system::error_code& ec) {
  auto vm_end_it = vm.end();
  auto stdin_it = vm.find("stdin");

  if (stdin_it != vm_end_it) {
    from_stdin_ = stdin_it->second.as<bool>();
  }

  auto arg1_it = vm.find("arg1");
  if (arg1_it == vm_end_it) {
    return;
  }

  ParseFirstArgument(vm["arg1"].as<std::string>(), ec);
  if (ec) {
    return;
  }

  auto arg2_it = vm.find("arg2");
  if (arg2_it != vm_end_it) {
    ParseSecondArgument(arg2_it->second.as<std::string>(), ec);
  }
}

std::string CopyCommandLine::GetUsageDesc() {
  std::stringstream ss_desc;
  ss_desc << exec_name_ << " [options] [host@]/absolute/path/file"
                           " [[host@]/absolute/path/file]";
  return ss_desc.str();
}

void CopyCommandLine::ParseFirstArgument(const std::string& first_arg,
                                         boost::system::error_code& parse_ec) {
  if (from_stdin_) {
    // Expecting host:filepath syntax
    ExtractHostPattern(first_arg, &host_, &output_pattern_, parse_ec);
    if (!parse_ec) {
      from_local_to_remote_ = true;
    }
  } else {
    // Expecting host:dirpath or filepath syntax
    boost::system::error_code extract_ec;
    ExtractHostPattern(first_arg, &host_, &input_pattern_, extract_ec);
    if (!extract_ec) {
      from_local_to_remote_ = false;
    } else {
      // Not host:dirpath syntax so it is filepath syntax
      input_pattern_ = first_arg;
      from_local_to_remote_ = true;
    }
  }
}

void CopyCommandLine::ParseSecondArgument(const std::string& second_arg,
                                          boost::system::error_code& parse_ec) {
  if (from_stdin_) {
    // No second arg should be provided with stdin option
    SSF_LOG(kLogError) << "command line: parsing failed: two args provided "
                          "with stdin option, one expected";
    parse_ec.assign(::error::invalid_argument, ::error::get_ssf_category());

    return;
  }

  // Expecting host:filepath or filepath syntax
  if (from_local_to_remote_) {
    // Expecting host:dirpath
    ExtractHostPattern(second_arg, &host_, &output_pattern_, parse_ec);
    if (parse_ec) {
      return;
    }

  } else {
    // Expecting dirpath
    output_pattern_ = second_arg;
  }

  // Insert trailing slash if not present
  auto last_slash_pos = output_pattern_.find_last_of('/');
  if (last_slash_pos != output_pattern_.size() - 1) {
    output_pattern_.append("/");
  }
}

void CopyCommandLine::ExtractHostPattern(const std::string& string,
                                         std::string* p_host,
                                         std::string* p_pattern,
                                         boost::system::error_code& ec) const {
  std::size_t found = string.find_first_of(kHostDirectorySeparator);
  if (found == std::string::npos || string.empty()) {
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return;
  }

  *p_host = string.substr(0, found);
  *p_pattern = string.substr(found + 1);
  ec.assign(::error::success, ::error::get_ssf_category());
}

}  // command_line
}  // ssf
