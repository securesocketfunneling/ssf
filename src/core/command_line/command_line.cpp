#include "command_line.h"

#include <cstdint>

#include <string>
#include <stdexcept>
#include <vector>
#include <regex>
#include <memory>

#include <boost/program_options.hpp>
#include <boost/system/error_code.hpp>

#include "common/error/error.h"

ssf::CommandLine::CommandLine(bool is_server): bounce_file_(""),
                                                       addr_set_(false),
                                                       port_set_(false),
                                                       is_server_(is_server) {}

std::map<std::string, std::vector<std::string>> ssf::CommandLine::parse(
    int argc, char* argv[], boost::system::error_code& ec) {
  boost::program_options::options_description services;
  return parse(argc, argv, services, ec);
}

std::map<std::string, std::vector<std::string>> ssf::CommandLine::parse(
    int ac, char *av[],
    const boost::program_options::options_description &services,
    boost::system::error_code &ec) {
  try {
    uint16_t opt;
    boost::program_options::options_description desc("Basic options");
    desc.add_options()
      ("help,h", "produce help message")
      ("version,v", "Version");

    boost::program_options::options_description options("Local options");
    options.add_options()(
      "port,p",
      boost::program_options::value<uint16_t>(&opt)->default_value(8011),
      "set port");

    if (!is_server_) {
      options.add_options()("host,H", boost::program_options::value<std::string>(),
                            "host");
    }

    boost::program_options::positional_options_description p;
    p.add("host", -1);

    options.add_options()("bounces,b",
                          boost::program_options::value<std::string>(),
                          "set bounce file");

    options.add_options()("config,c",
                          boost::program_options::value<std::string>(),
                          "set config file");

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

    ec.assign(ssf::error::success, ssf::error::get_ssf_category());
    return InternalParsing(vm);
  }
  catch (const std::exception&) {
    ec.assign(ssf::error::invalid_argument,
              ssf::error::get_ssf_category());
    return std::map<std::string, std::vector<std::string>>();
  }
}

uint16_t ssf::CommandLine::port() {
  return port_;
}

std::string ssf::CommandLine::addr() {
  return addr_;
}

std::string ssf::CommandLine::bounce_file() {
  return bounce_file_;
}

std::string ssf::CommandLine::config_file() {
  return config_file_;
}

bool ssf::CommandLine::IsPortSet() {
  return port_set_;
}

bool ssf::CommandLine::IsAddrSet() {
  return addr_set_;
}

std::map<std::string, std::vector<std::string>> ssf::CommandLine::InternalParsing(
    const boost::program_options::variables_map& vm) {
  std::map<std::string, std::vector<std::string>> result;

  for (auto& variable : vm) {
    if (variable.first == "help") {
    } else if (variable.first == "version") {
    } else if (variable.first == "port") {
      port_ = vm[variable.first].as<uint16_t>();
      port_set_ = true;
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
