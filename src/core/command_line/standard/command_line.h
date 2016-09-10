#ifndef SSF_CORE_COMMAND_LINE_STANDARD_COMMAND_LINE_H
#define SSF_CORE_COMMAND_LINE_STANDARD_COMMAND_LINE_H

#include <cstdint>

#include <string>
#include <stdexcept>
#include <vector>
#include <regex>
#include <memory>

#include <boost/program_options.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "core/command_line/base.h"

namespace ssf {
namespace command_line {
namespace standard {

class CommandLine : public BaseCommandLine {
 public:
  using ParsedParameters = std::map<std::string, std::vector<std::string>>;

 public:
  CommandLine(bool is_server = false);

  inline bool show_status() const { return show_status_; }

  inline bool relay_only() const { return relay_only_; }

  inline bool gateway_ports() const { return gateway_ports_; }

 protected:
  void PopulateBasicOptions(OptionDescription& desc) override;
  void PopulateLocalOptions(OptionDescription& desc) override;
  void PopulatePositionalOptions(PosOptionDescription& desc) override;
  void PopulateCommandLine(OptionDescription& command_line) override;
  bool IsServerCli() override;
  void ParseOptions(const VariableMap& vm, ParsedParameters& parsed_params,
                    boost::system::error_code& ec) override;
  std::string GetUsageDesc() override;

 private:
  bool is_server_;
  bool show_status_;
  bool relay_only_;
  bool gateway_ports_;
};

}  // standard
}  // command_line
}  // ssf

#endif  // SSF_CORE_COMMAND_LINE_STANDARD_COMMAND_LINE_H
