#ifndef SSF_CORE_COMMAND_LINE_BASE_H
#define SSF_CORE_COMMAND_LINE_BASE_H

#include <cstdint>

#include <string>
#include <stdexcept>
#include <vector>
#include <regex>
#include <memory>

#include <boost/program_options.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

namespace ssf {
namespace command_line {

class BaseCommandLine {
 public:
  using OptionDescription = boost::program_options::options_description;
  using PosOptionDescription =
      boost::program_options::positional_options_description;
  using VariableMap = boost::program_options::variables_map;
  using ParsedParameters = std::map<std::string, std::vector<std::string>>;

 public:
  virtual ~BaseCommandLine() {}

  ParsedParameters Parse(int argc, char* argv[], boost::system::error_code& ec);

  ParsedParameters Parse(int ac, char* av[], const OptionDescription& services,
                         boost::system::error_code& ec);

  inline std::string host() const { return host_; }

  inline uint16_t port() const { return port_; };

  inline std::string circuit_file() const { return circuit_file_; }

  inline std::string config_file() const { return config_file_; }

  inline ssf::log::LogLevel log_level() const { return log_level_; }

  inline bool host_set() const { return host_set_; }

  inline bool port_set() const { return port_set_; }

 protected:
  BaseCommandLine();

  virtual void PopulateBasicOptions(OptionDescription& basic_opts);
  virtual void PopulateLocalOptions(OptionDescription& local_opts);
  virtual void PopulatePositionalOptions(PosOptionDescription& pos_opts);
  virtual void PopulateCommandLine(OptionDescription& command_line);

  virtual bool IsServerCli() = 0;
  virtual void ParseOptions(const VariableMap& value,
                            ParsedParameters& parsed_params,
                            boost::system::error_code& ec) = 0;

  static bool DisplayHelp(const VariableMap& vm, const OptionDescription& cli);

  void ParseBaseOptions(const VariableMap& vm, boost::system::error_code& ec);

  ParsedParameters DoParse(const OptionDescription& services,
                           const VariableMap& vm,
                           boost::system::error_code& ec);

 private:
  void InitBasicOptions(OptionDescription& basic_opts);
  void InitLocalOptions(OptionDescription& local_opts);

  void set_log_level(const std::string& level);

 protected:
  std::string host_;
  uint16_t port_;
  std::string config_file_;
  std::string circuit_file_;
  ssf::log::LogLevel log_level_;
  bool host_set_;
  bool port_set_;
};

}  // command_line
}  // ssf

#endif  // SSF_CORE_COMMAND_LINE_BASE_H
