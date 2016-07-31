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

  virtual ~CommandLine() {}

 protected:
  void PopulateBasicOptions(OptionDescription& desc) override;
  void PopulateLocalOptions(OptionDescription& desc) override;
  void PopulatePositionalOptions(PosOptionDescription& desc) override;
  void PopulateCommandLine(OptionDescription& command_line) override;
  bool IsServerCli() override;
  void ParseOptions(const VariableMap& vm, ParsedParameters& parsed_params,
                    boost::system::error_code& ec) override;

 private:
  bool is_server_;
};

}  // standard
}  // command_line
}  // ssf

#endif  // SSF_CORE_COMMAND_LINE_STANDARD_COMMAND_LINE_H
