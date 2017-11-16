#ifndef SSF_CORE_COMMAND_LINE_BASE_H
#define SSF_CORE_COMMAND_LINE_BASE_H

#include <cstdint>

#include <map>
#include <string>
#include <vector>

#include <cxxopts.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "services/user_services/parameters.h"

namespace ssf {

class UserServiceOptionFactory;

namespace command_line {

class Base {
 public:
  using Options = cxxopts::Options;

 public:
  virtual ~Base() {}

  UserServiceParameters Parse(int argc, char* argv[],
                              boost::system::error_code& ec);

  UserServiceParameters Parse(
      int argc, char* argv[],
      const UserServiceOptionFactory& user_service_option_factory,
      boost::system::error_code& ec);

  inline std::string host() const { return host_; }

  inline uint16_t port() const { return port_; };

  inline std::string config_file() const { return config_file_; }

  inline spdlog::level::level_enum log_level() const { return log_level_; }

  inline bool host_set() const { return !host_.empty(); }

  inline bool port_set() const { return port_set_; }

 protected:
  Base();

  virtual void ParseOptions(const Options& opts,
                            boost::system::error_code& ec);

  // Return true for server CLI
  virtual bool IsServerCli();

  virtual void InitOptions(Options& opts);

 private:
  bool DisplayHelp(const Options& opts);

  void ParseBasicOptions(const Options& opts, boost::system::error_code& ec);

  UserServiceParameters DoParse(
      const UserServiceOptionFactory& user_service_option_factory,
      const Options& opts, boost::system::error_code& ec);

  void set_log_level(const std::string& level);

 protected:
  std::string exec_name_;
  std::string host_;
  uint16_t port_;
  std::string config_file_;
  spdlog::level::level_enum log_level_;
  bool port_set_;
};

}  // command_line
}  // ssf

#endif  // SSF_CORE_COMMAND_LINE_BASE_H
