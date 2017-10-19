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
static const uint32_t kMaxParallelCopies = 1;

CopyCommandLine::CopyCommandLine()
    : Base(),
      from_client_to_server_(true),
      stdin_input_(false),
      resume_(false),
      recursive_(false),
      check_file_integrity_(false),
      max_parallel_copies_(kMaxParallelCopies) {}

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
    ("stdin-input,t",
        boost::program_options::bool_switch()
          ->value_name("stdin-input")
          ->default_value(false),
          "File input will be stdin")
    ("resume",
        boost::program_options::bool_switch()
          ->value_name("resume")
          ->default_value(false),
          "If the output file exists, this option will try to resume the copy")
    ("check-integrity",
        boost::program_options::bool_switch()
          ->default_value(false),
          "Check file integrity after copy")
    ("recursive,r",
        boost::program_options::bool_switch()
          ->value_name("recursive")
          ->default_value(false),
          "Copy files recursively. Not available")
    ("max-parallel-copies",
        boost::program_options::value<uint32_t>()
          ->value_name("max-parallel-copies")
          ->default_value(kMaxParallelCopies),
          "Max copies in parallel")
    ("arg1",
        boost::program_options::value<std::string>()
          ->value_name("[host@]/absolute/path/file"),
          "[host@]/absolute/path/file if host is present, " \
          "the file will be copied from server to client")
    ("arg2",
        boost::program_options::value<std::string>()
          ->value_name("[host@]/absolute/path/file"),
          "[host@]/absolute/path/file if host is present, " \
          "the file will be copied from client to server");
  // clang-format on

  command_line.add(copy_options);
}

bool CopyCommandLine::IsServerCli() { return false; }

bool CopyCommandLine::stdin_input() const { return stdin_input_; }

bool CopyCommandLine::from_client_to_server() const {
  return from_client_to_server_;
}

bool CopyCommandLine::resume() const { return resume_; }

bool CopyCommandLine::recursive() const { return recursive_; }

bool CopyCommandLine::check_file_integrity() const {
  return check_file_integrity_;
}

uint32_t CopyCommandLine::max_parallel_copies() const {
  return max_parallel_copies_;
}

std::string CopyCommandLine::input_pattern() const { return input_pattern_; }

std::string CopyCommandLine::output_pattern() const { return output_pattern_; }

void CopyCommandLine::ParseOptions(const VariableMap& vm,
                                   boost::system::error_code& ec) {
  auto vm_end_it = vm.end();
  stdin_input_ = vm["stdin-input"].as<bool>();
  if (!stdin_input_) {
    resume_ = vm["resume"].as<bool>();
    recursive_ = vm["recursive"].as<bool>();
    check_file_integrity_ = vm["check-integrity"].as<bool>();
    max_parallel_copies_ = vm["max-parallel-copies"].as<uint32_t>();
    if (max_parallel_copies_ == 0) {
      SSF_LOG(kLogError) << "[cli] max-parallel-copies must be > 0";
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      return;
    }
  }

  auto arg1_it = vm.find("arg1");
  if (arg1_it == vm_end_it) {
    return;
  }

  ParseFirstArgument(arg1_it->second.as<std::string>(), ec);
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
  if (stdin_input_) {
    // Expecting host:filepath syntax
    ExtractHostPattern(first_arg, &host_, &output_pattern_, parse_ec);
    if (!parse_ec) {
      from_client_to_server_ = true;
    }
  } else {
    // Expecting host:dirpath or filepath syntax
    boost::system::error_code extract_ec;
    ExtractHostPattern(first_arg, &host_, &input_pattern_, extract_ec);
    if (!extract_ec) {
      from_client_to_server_ = false;
    } else {
      // Not host:dirpath syntax so it is filepath syntax
      input_pattern_ = first_arg;
      from_client_to_server_ = true;
    }
  }
}

void CopyCommandLine::ParseSecondArgument(const std::string& second_arg,
                                          boost::system::error_code& parse_ec) {
  if (stdin_input_) {
    // No second arg should be provided with stdin option
    SSF_LOG(kLogError) << "[cli] parsing failed: two args provided "
                          "with stdin option, one expected";
    parse_ec.assign(::error::invalid_argument, ::error::get_ssf_category());

    return;
  }

  // Expecting host:filepath or filepath syntax
  if (from_client_to_server_) {
    // Expecting host:dirpath
    ExtractHostPattern(second_arg, &host_, &output_pattern_, parse_ec);
    if (parse_ec) {
      return;
    }

  } else {
    // Expecting dirpath
    output_pattern_ = second_arg;
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
