#include "core/command_line/base.h"

#include <iostream>
#include <stdexcept>

#include <boost/filesystem.hpp>

#include <ssf/log/log.h>

#include "common/error/error.h"

#include "core/command_line/user_service_option_factory.h"

#include "versions.h"

namespace ssf {
namespace command_line {

Base::Base()
    : host_(""),
      port_(0),
      config_file_(""),
      log_level_(spdlog::level::info),
      port_set_(false) {}

UserServiceParameters Base::Parse(int argc, char* argv[],
                                  boost::system::error_code& ec) {
  UserServiceOptionFactory user_service_option_factory;
  return Parse(argc, argv, user_service_option_factory, ec);
}

UserServiceParameters Base::Parse(
    int argc, char* argv[],
    const UserServiceOptionFactory& user_service_option_factory,
    boost::system::error_code& ec) {
  // Set executable name
  if (argc > 0) {
    boost::filesystem::path path(argv[0]);
    exec_name_ = path.filename().string();
  }

  Options opts(argv[0], "Secure Socket Funneling " SSF_VERSION_STRING);
  opts.positional_help("");

  InitOptions(opts);

  user_service_option_factory.InitOptions(opts);

  opts.parse(argc, argv);

  if (DisplayHelp(opts)) {
    ec.assign(::error::operation_canceled, ::error::get_ssf_category());
    return {};
  }

  return DoParse(user_service_option_factory, opts, ec);
}

void Base::ParseOptions(const Options& opts,
                        boost::system::error_code& ec) {}

bool Base::IsServerCli() { return false; }

bool Base::DisplayHelp(const Options& opts) {
  if (!opts.count("help")) {
    return false;
  }

  std::cerr << opts.help(opts.groups()) << std::endl;

  std::cerr << "Using Boost " << ssf::versions::boost_version << " and OpenSSL "
            << ssf::versions::openssl_version << std::endl
            << std::endl;

  return true;
}

void Base::ParseBasicOptions(const Options& opts,
                             boost::system::error_code& ec) {
  log_level_ = spdlog::level::info;
  port_ = 0;
  port_set_ = false;
  config_file_ = "";

  if (opts.count("quiet")) {
    log_level_ = spdlog::level::off;
  } else {
    set_log_level(opts["verbosity"].as<std::string>());
  }

  int port = opts["port"].as<int>();

  if (port > 0 && port < 65536) {
    port_ = static_cast<uint16_t>(port);
    port_set_ = true;
  } else {
    SSF_LOG("cli", error, "parsing failed: port option is not "
           "between 1 - 65536");
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
  }

  try {
    config_file_ = opts["config"].as<std::string>();
  } catch (cxxopts::option_not_present_exception) { }
}

UserServiceParameters Base::DoParse(
    const UserServiceOptionFactory& user_service_option_factory,
    const Options& opts, boost::system::error_code& ec) {
  UserServiceParameters result;

  ParseBasicOptions(opts, ec);
  if (ec) {
    return {};
  }

  ParseOptions(opts, ec);
  if (ec) {
    return {};
  }

  return user_service_option_factory.CreateUserServiceParameters(opts, ec);
}

void Base::InitOptions(Options& opts) {
  // clang-format off
  opts.add_options()
    ("h,help", "Show help message");

  opts.add_options()
    ("v,verbosity",
        "Verbosity: critical|error|warning|info|debug|trace",
        cxxopts::value<std::string>()->default_value("info"));

  opts.add_options()
    ("q,quiet", "Do not print logs");

  opts.add_options()
    ("c,config",
        "Specify configuration file. If not set, 'config.json' is loaded"
        " from the current working directory",
        cxxopts::value<std::string>());

  if (!IsServerCli()) {
    opts.add_options()
      ("p,port",
          "Remote port",
          cxxopts::value<int>()->default_value("8011"));
  } else {
    opts.add_options()
      ("p,port",
          "Local port",
          cxxopts::value<int>()->default_value("8011"));
  }
  // clang-format on
}

void Base::set_log_level(const std::string& level) {
  if (log_level_ == spdlog::level::off) {
    // Quiet set
    return;
  }

  if (level == "critical") {
    log_level_ = spdlog::level::critical;
  } else if (level == "error") {
    log_level_ = spdlog::level::err;
  } else if (level == "warning") {
    log_level_ = spdlog::level::warn;
  } else if (level == "info") {
    log_level_ = spdlog::level::info;
  } else if (level == "debug") {
    log_level_ = spdlog::level::debug;
  } else if (level == "trace") {
    log_level_ = spdlog::level::trace;
  } else {
    log_level_ = spdlog::level::info;
  }
}

}  // command_line
}  // ssf
