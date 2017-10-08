#ifndef SSF_CORE_COMMAND_LINE_STANDARD_COMMAND_LINE_H
#define SSF_CORE_COMMAND_LINE_STANDARD_COMMAND_LINE_H

#include <ssf/log/log.h>

#include "core/command_line/base.h"

namespace ssf {
namespace command_line {

class StandardCommandLine : public Base {
 public:
  StandardCommandLine(bool is_server = false);

  ~StandardCommandLine();

  bool show_status() const { return show_status_; }

  bool relay_only() const { return relay_only_; }

  bool gateway_ports() const { return gateway_ports_; }

  uint32_t max_connection_attempts() const { return max_connection_attempts_; }

  uint32_t reconnection_timeout() const { return reconnection_timeout_; }

  bool no_reconnection() const { return no_reconnection_; }

 protected:
  void PopulateBasicOptions(OptionDescription& desc) override;
  void PopulateLocalOptions(OptionDescription& desc) override;
  void PopulatePositionalOptions(PosOptionDescription& desc) override;
  void PopulateCommandLine(OptionDescription& command_line) override;
  bool IsServerCli() override;
  void ParseOptions(const VariableMap& vm,
                    boost::system::error_code& ec) override;
  std::string GetUsageDesc() override;

 private:
  bool is_server_;
  bool show_status_;
  bool relay_only_;
  bool gateway_ports_;
  uint32_t max_connection_attempts_;
  uint32_t reconnection_timeout_;
  bool no_reconnection_;
};

}  // command_line
}  // ssf

#endif  // SSF_CORE_COMMAND_LINE_STANDARD_COMMAND_LINE_H
