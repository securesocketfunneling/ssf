#include "core/command_line/standard/command_line.h"

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
namespace standard {

CommandLine::CommandLine(bool is_server)
    : bounce_file_(""),
      addr_set_(false),
      port_set_(false),
      is_server_(is_server) {}

CommandLine::ParsedParameters CommandLine::parse(
    int argc, char* argv[], boost::system::error_code& ec) {
  boost::program_options::options_description services;
  return parse(argc, argv, services, ec);
}

CommandLine::ParsedParameters CommandLine::parse(
    int ac, char* av[],
    const boost::program_options::options_description& services,
    boost::system::error_code& ec) {
  try {
    // clang-format off
    int opt;
    boost::program_options::options_description desc("Basic options");
    desc.add_options()
      ("help,h", "Produce help message");

    boost::program_options::options_description options("Local options");

    boost::program_options::positional_options_description p;

    options.add_options()
        ("host,H",
            boost::program_options::value<std::string>(),
            "Set host");

    p.add("host", 1);
    options.add_options()
      ("config,c",
          boost::program_options::value<std::string>(),
          "Set config file");

    if (!is_server_) {
      options.add_options()
        ("port,p",
            boost::program_options::value<int>(&opt)->default_value(8011),
            "Set remote SSF server port");

      options.add_options()
        ("bounces,b",
            boost::program_options::value<std::string>(),
            "Set bounce file");
    } else {
      options.add_options()
        ("port,p",
            boost::program_options::value<int>(&opt)->default_value(8011),
            "Set local SSF server port");
    }
    // clang-format off

    boost::program_options::options_description cmd_line;
    cmd_line.add(desc).add(options).add(services);

    boost::program_options::variables_map vm;
    boost::program_options::store(
        boost::program_options::command_line_parser(ac, av)
            .options(cmd_line)
            .positional(p)
            .run(),
        vm);
    boost::program_options::notify(vm);

    if (vm.count("help")) {
      std::cout << cmd_line << std::endl;
    }

    ec.assign(::error::success, ::error::get_ssf_category());
    return InternalParsing(vm, ec);
  } catch (const std::exception&) {
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return std::map<std::string, std::vector<std::string>>();
  }
}

uint16_t CommandLine::port() { return port_; }

std::string CommandLine::addr() { return addr_; }

std::string CommandLine::bounce_file() { return bounce_file_; }

std::string CommandLine::config_file() { return config_file_; }

bool CommandLine::IsPortSet() { return port_set_; }

bool CommandLine::IsAddrSet() { return addr_set_; }

CommandLine::ParsedParameters CommandLine::InternalParsing(
    const boost::program_options::variables_map& vm,
    boost::system::error_code& ec) {
  std::map<std::string, std::vector<std::string>> result;

  for (auto& variable : vm) {
    if (variable.first == "help") {
    } else if (variable.first == "port") {
      int port = vm[variable.first].as<int>();
      if (port > 0 && port < 65536) {
        port_ = static_cast<uint16_t>(port);
        port_set_ = true;
      } else {
        ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      }
    } else if (variable.first == "host") {
      addr_ = vm[variable.first].as<std::string>();
      addr_set_ = true;
    } else if (variable.first == "bounces") {
      bounce_file_ = vm[variable.first].as<std::string>();
    } else if (variable.first == "config") {
      config_file_ = vm[variable.first].as<std::string>();
    } else {
      result[variable.first] =
          vm[variable.first].as<std::vector<std::string>>();
    }
  }

  return result;
}

}  // standard
}  // command_line
}  // ssf
