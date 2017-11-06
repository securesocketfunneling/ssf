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

CopyCommandLine::CopyCommandLine()
    : Base(),
      from_client_to_server_(true),
      stdin_input_(false),
      resume_(false),
      recursive_(false),
      check_file_integrity_(false),
      max_parallel_copies_() {}

void CopyCommandLine::InitOptions(Options& opts) {
  // clang-format off
  Base::InitOptions(opts);

  opts.add_options("Copy")
    ("t,stdin-input", "Use stdin as input")
    ("resume", "Attempt to resume operation if the destination file exists")
    ("check-integrity", "Check file integrity")
    ("r,recursive", "Copy files recursively")
    ("max-transfers", "Number of transfers in parallel",
        cxxopts::value<uint32_t>()->default_value("1"))
    ("args", "", cxxopts::value<std::vector<std::string>>());

  opts.parse_positional("args");
  opts.positional_help("[host@]source_path [host@]destination_path");

  // clang-format on
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

void CopyCommandLine::ParseOptions(const Options& opts,
                                   boost::system::error_code& ec) {
  stdin_input_ = opts.count("stdin-input");

  if (!stdin_input_) {
    resume_ = opts.count("resume");
    recursive_ = opts.count("recursive");
    check_file_integrity_ = opts.count("check-integrity");
    max_parallel_copies_ = opts["max-transfers"].as<uint32_t>();
    if (max_parallel_copies_ == 0) {
      SSF_LOG("cli", error, "max-transfers must be > 0");
      ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      return;
    }
  }

  auto& args = opts["args"].as<std::vector<std::string>>();

  switch (args.size()) {
  case 0:
    SSF_LOG("cli", error, "missing arguments");
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return;
  case 1:
    ParseFirstArgument(args[0], ec);
    if (ec) {
      return;
    }
    break;
  case 2:
    ParseFirstArgument(args[0], ec);
    if (ec) {
      return;
    }
    ParseSecondArgument(args[1], ec);
    if (ec)
      return;
    break;
  default:
    SSF_LOG("cli", error, "too many arguments");
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return;
  }
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
    SSF_LOG(
        "cli", error,
        "parsing failed: two args provided with stdin option, one expected");
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
