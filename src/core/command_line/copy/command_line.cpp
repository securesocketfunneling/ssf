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

namespace ssf {
namespace command_line {
namespace copy {

CommandLine::CommandLine()
    : addr_(""), bounce_file_(""), addr_set_(false), port_set_(false) {}

void CommandLine::parse(int argc, char* argv[], boost::system::error_code& ec) {
  try {
    // clang-format off
    int opt;
    boost::program_options::options_description desc("Basic options");
    desc.add_options()
      ("help,h", "Produce help message");

    boost::program_options::options_description options("Client options");
    options.add_options()
      ("port,p",
          boost::program_options::value<int>(&opt)->default_value(8011),
          "Set remote SSF server port")
      ("bounces,b",
          boost::program_options::value<std::string>(),
          "Set bounce file")
      ("config,c",
          boost::program_options::value<std::string>(),
          "Set config file");

    boost::program_options::options_description copy_options("Copy options");
    copy_options.add_options()
      ("stdin,t", boost::program_options::bool_switch()->default_value(false), "Input will be stdin")
      ("arg1",
         boost::program_options::value<std::string>(),
         "[host:]/absolute/path/file if host is present, the file will be copied from the remote host to local")
      ("arg2",
         boost::program_options::value<std::string>(),
         "[host:]/absolute/path/file if host is present, the file will be copied from local to host");
    // clang-format on

    boost::program_options::positional_options_description position_options;
    position_options.add("arg1", 1);
    position_options.add("arg2", 1);

    boost::program_options::options_description cmd_line;
    cmd_line.add(desc).add(options).add(copy_options);
    boost::program_options::variables_map vm;
    boost::program_options::store(
        boost::program_options::command_line_parser(argc, argv)
            .options(cmd_line)
            .positional(position_options)
            .run(),
        vm);
    boost::program_options::notify(vm);

    if (vm.count("help")) {
      std::cout << "usage : ssfcp  [-p port] [-b bounces_file] [-c "
                   "config] [-h] [-t] arg1 [arg2]" << std::endl;
      std::cout << cmd_line << std::endl;
    }

    ec.assign(ssf::error::success, ssf::error::get_ssf_category());
    InternalParsing(vm, ec);
  } catch (const std::exception&) {
    ec.assign(ssf::error::invalid_argument, ssf::error::get_ssf_category());
  }
}

uint16_t CommandLine::port() const { return port_; }

std::string CommandLine::addr() const { return addr_; }

std::string CommandLine::bounce_file() const { return bounce_file_; }

std::string CommandLine::config_file() const { return config_file_; }

bool CommandLine::from_stdin() const { return from_stdin_; }

bool CommandLine::from_local_to_remote() const { return from_local_to_remote_; }

std::string CommandLine::input_pattern() const { return input_pattern_; }

std::string CommandLine::output_pattern() const { return output_pattern_; }

bool CommandLine::IsPortSet() const { return port_set_; }

bool CommandLine::IsAddrSet() const { return addr_set_; }

void CommandLine::InternalParsing(
    const boost::program_options::variables_map& vm,
    boost::system::error_code& ec) {
  auto port_it = vm.find("port");
  if (port_it != vm.end()) {
    ParsePort(port_it->second.as<int>(), ec);
  }

  auto stdin_it = vm.find("stdin");
  if (stdin_it != vm.end()) {
    from_stdin_ = stdin_it->second.as<bool>();
  }

  auto bounces_file_it = vm.find("bounces");
  if (bounces_file_it != vm.end()) {
    bounce_file_ = bounces_file_it->second.as<std::string>();
  }

  auto config_it = vm.find("config");
  if (config_it != vm.end()) {
    config_file_ = config_it->second.as<std::string>();
  }

  auto first_arg_it = vm.find("arg1");
  if (first_arg_it != vm.end()) {
    ParseFirstArgument(first_arg_it->second.as<std::string>(), ec);
  } else {
    // there should always exist a first arg (input file, or host:input_file or
    // host:output_file)
    ec.assign(ssf::error::invalid_argument, ssf::error::get_ssf_category());
    return;
  }

  auto second_arg_it = vm.find("arg2");
  if (second_arg_it != vm.end()) {
    ParseSecondArgument(second_arg_it->second.as<std::string>(), ec);
  }
}

void CommandLine::ParsePort(int port, boost::system::error_code& parse_ec) {
  if (port > 0 && port < 65536) {
    port_ = static_cast<uint16_t>(port);
    port_set_ = true;
  } else {
    parse_ec.assign(ssf::error::invalid_argument,
                    ssf::error::get_ssf_category());
  }
}

void CommandLine::ParseFirstArgument(const std::string& first_arg,
                                     boost::system::error_code& parse_ec) {
  if (from_stdin_) {
    // expecting host:filepath syntax
    ExtractHostPattern(first_arg, &addr_, &output_pattern_, parse_ec);
    if (!parse_ec) {
      addr_set_ = true;
      from_local_to_remote_ = true;
    }
  } else {
    // expecting host:dirpath or filepath syntax
    boost::system::error_code extract_ec;
    ExtractHostPattern(first_arg, &addr_, &input_pattern_, extract_ec);
    if (!extract_ec) {
      addr_set_ = true;
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
    parse_ec.assign(ssf::error::invalid_argument,
                    ssf::error::get_ssf_category());
  } else {
    // expecting host:filepath or filepath syntax
    if (from_local_to_remote_) {
      // expecting host:dirpath
      ExtractHostPattern(second_arg, &addr_, &output_pattern_, parse_ec);
      if (parse_ec) {
        addr_set_ = false;
        return;
      }

      addr_set_ = true;
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
}

void CommandLine::ExtractHostPattern(const std::string& string,
                                     std::string* p_host,
                                     std::string* p_pattern,
                                     boost::system::error_code& ec) const {
  std::size_t found = string.find_first_of(GetHostDirectorySeparator());
  if (found == std::string::npos || string.empty()) {
    ec.assign(ssf::error::invalid_argument, ssf::error::get_ssf_category());
    return;
  }

  *p_host = string.substr(0, found);
  *p_pattern = string.substr(found + 1);
  ec.assign(ssf::error::success, ssf::error::get_ssf_category());
}

char CommandLine::GetHostDirectorySeparator() const { return '@'; }

}  // copy
}  // command_line
}  // ssf
