#ifndef SSF_CORE_COMMAND_LINE_COPY_COMMAND_LINE_H_
#define SSF_CORE_COMMAND_LINE_COPY_COMMAND_LINE_H_

#include <string>

#include "core/command_line/base.h"

namespace ssf {
namespace command_line {
namespace copy {

class CommandLine : public BaseCommandLine {
 public:
  CommandLine();

  ~CommandLine();

  bool from_stdin() const;

  bool from_local_to_remote() const;

  std::string input_pattern() const;

  std::string output_pattern() const;

 protected:
  void PopulateBasicOptions(OptionDescription& desc) override;
  void PopulateLocalOptions(OptionDescription& desc) override;
  void PopulatePositionalOptions(PosOptionDescription& desc) override;
  void PopulateCommandLine(OptionDescription& command_line) override;
  bool IsServerCli() override;
  void ParseOptions(const VariableMap& value, ParsedParameters& parsed_params,
                    boost::system::error_code& ec) override;
  std::string GetUsageDesc() override;

 private:
  void ParseFirstArgument(const std::string& first_arg,
                          boost::system::error_code& parse_ec);

  void ParseSecondArgument(const std::string& second_arg,
                           boost::system::error_code& parse_ec);

  void ExtractHostPattern(const std::string& string, std::string* p_host,
                          std::string* p_pattern,
                          boost::system::error_code& ec) const;

  char GetHostDirectorySeparator() const;

 private:
  std::string input_pattern_;
  std::string output_pattern_;
  bool from_stdin_;
  bool from_local_to_remote_;
};

}  // copy
}  // command_line
}  // ssf

#endif  // SSF_CORE_COMMAND_LINE_COPY_COMMAND_LINE_H_
