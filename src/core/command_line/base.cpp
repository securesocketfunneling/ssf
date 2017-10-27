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
      log_level_(ssf::log::kLogInfo),
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

  // Basic options
  OptionDescription basic_opts("Basic options");
  InitBasicOptions(basic_opts);
  PopulateBasicOptions(basic_opts);

  // Local options
  OptionDescription local_opts("Local options");
  InitLocalOptions(local_opts);
  PopulateLocalOptions(local_opts);

  try {
    OptionDescription cmd_line;

    cmd_line.add(basic_opts).add(local_opts);

    // Other options
    PopulateCommandLine(cmd_line);

    // Positional options
    PosOptionDescription pos_opts;
    PopulatePositionalOptions(pos_opts);

    cmd_line.add(user_service_option_factory.GetOptions());

    VariableMap vm;
    boost::program_options::store(
        boost::program_options::command_line_parser(argc, argv)
            .options(cmd_line)
            .positional(pos_opts)
            .run(),
        vm);

    if (DisplayHelp(vm, cmd_line)) {
      ec.assign(::error::operation_canceled, ::error::get_ssf_category());
      return {};
    }

    boost::program_options::notify(vm);

    return DoParse(user_service_option_factory, vm, ec);
  } catch (const std::exception& e) {
    SSF_LOG(kLogCritical) << "[cli] parsing failed: " << e.what();
    ec.assign(::error::invalid_argument, ::error::get_ssf_category());
    return {};
  }
}

void Base::PopulateBasicOptions(OptionDescription& basic_opts) {}

void Base::PopulateLocalOptions(OptionDescription& local_opts) {}

void Base::PopulatePositionalOptions(PosOptionDescription& pos_opts) {}

void Base::PopulateCommandLine(OptionDescription& command_line) {}

void Base::ParseOptions(const VariableMap& value,
                        boost::system::error_code& ec) {}

bool Base::IsServerCli() { return false; }

bool Base::DisplayHelp(const VariableMap& vm, const OptionDescription& cli) {
  if (!vm.count("help")) {
    return false;
  }

  std::cout << "SSF " << ssf::versions::major << "." << ssf::versions::minor
            << "." << ssf::versions::fix << std::endl
            << std::endl;

  std::cout << "Usage: " << GetUsageDesc() << std::endl;

  std::cout << cli << std::endl;

  std::cout << "Using Boost " << ssf::versions::boost_version << " and OpenSSL "
            << ssf::versions::openssl_version << std::endl
            << std::endl;

  return true;
}

void Base::ParseBasicOptions(const VariableMap& vm,
                             boost::system::error_code& ec) {
  log_level_ = ssf::log::kLogInfo;
  port_ = 0;
  port_set_ = false;
  config_file_ = "";

  for (const auto& option : vm) {
    if (option.first == "quiet") {
      log_level_ = ssf::log::kLogNone;
    } else if (option.first == "verbosity") {
      set_log_level(option.second.as<std::string>());
    } else if (option.first == "port") {
      int port = option.second.as<int>();
      if (port > 0 && port < 65536) {
        port_ = static_cast<uint16_t>(port);
        port_set_ = true;
      } else {
        SSF_LOG(kLogError)
            << "[cli] parsing failed: port option is not "
               "between 1 - 65536";
        ec.assign(::error::invalid_argument, ::error::get_ssf_category());
      }
    } else if (option.first == "config") {
      config_file_ = option.second.as<std::string>();
    }
  }
}

UserServiceParameters Base::DoParse(
    const UserServiceOptionFactory& user_service_option_factory,
    const VariableMap& vm, boost::system::error_code& ec) {
  UserServiceParameters result;

  ParseBasicOptions(vm, ec);
  if (ec) {
    return {};
  }

  ParseOptions(vm, ec);
  if (ec) {
    return {};
  }

  for (const auto& option : vm) {
    if (!user_service_option_factory.HasService(option.first)) {
      continue;
    }

    auto service_options = option.second.as<std::vector<std::string>>();
    for (const auto& service_option : service_options) {
      auto service_params =
          user_service_option_factory.CreateUserServiceParameters(
              option.first, service_option, ec);
      if (ec) {
        return {};
      }
      result[option.first].emplace_back(service_params);
    }
  }

  ec.assign(::error::success, ::error::get_ssf_category());

  return result;
}

void Base::InitBasicOptions(OptionDescription& basic_opts) {
  // clang-format off
  basic_opts.add_options()
    ("help,h", "Show help message");

  basic_opts.add_options()
    ("verbosity,v",
        boost::program_options::value<std::string>()
          ->value_name("level")
          ->default_value("info"),
        "Verbosity:\n  critical|error|warning|info|debug|trace");

  basic_opts.add_options()
    ("quiet,q", "Do not print logs");
  // clang-format on
}

void Base::InitLocalOptions(OptionDescription& local_opts) {
  // clang-format off
  local_opts.add_options()
    ("config,c",
        boost::program_options::value<std::string>()
          ->value_name("config_file_path"),
        "Specify configuration file. If not set, 'config.json' is loaded"
        " from the current working directory");

  if (!IsServerCli()) {
    local_opts.add_options()
      ("port,p",
          boost::program_options::value<int>()->default_value(8011)
            ->value_name("port"),
          "Remote port");
  } else {
    local_opts.add_options()
      ("port,p",
          boost::program_options::value<int>()->default_value(8011)
            ->value_name("port"),
          "Local port");
  }
  // clang-format on
}

void Base::set_log_level(const std::string& level) {
  if (log_level_ == ssf::log::kLogNone) {
    // Quiet set
    return;
  }

  if (level == "critical") {
    log_level_ = ssf::log::kLogCritical;
  } else if (level == "error") {
    log_level_ = ssf::log::kLogError;
  } else if (level == "warning") {
    log_level_ = ssf::log::kLogWarning;
  } else if (level == "info") {
    log_level_ = ssf::log::kLogInfo;
  } else if (level == "debug") {
    log_level_ = ssf::log::kLogDebug;
  } else if (level == "trace") {
    log_level_ = ssf::log::kLogTrace;
  } else {
    log_level_ = ssf::log::kLogInfo;
  }
}

}  // command_line
}  // ssf
