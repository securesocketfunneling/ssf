#ifndef SSF_CORE_COMMAND_LINE_COPY_COMMAND_LINE_H_
#define SSF_CORE_COMMAND_LINE_COPY_COMMAND_LINE_H_

#include <string>

#include "core/command_line/base.h"

namespace ssf {
namespace command_line {

class CopyCommandLine : public Base {
 public:
  CopyCommandLine();

  bool stdin_input() const;

  bool from_client_to_server() const;

  bool resume() const;

  bool recursive() const;

  bool check_file_integrity() const;

  uint32_t max_parallel_copies() const;

  std::string input_pattern() const;

  std::string output_pattern() const;

 protected:
  bool IsServerCli() override;
  void ParseOptions(const Options& opts,
                    boost::system::error_code& ec) override;
  void InitOptions(Options& opts) override;

 private:
  void ParseFirstArgument(const std::string& first_arg,
                          boost::system::error_code& parse_ec);

  void ParseSecondArgument(const std::string& second_arg,
                           boost::system::error_code& parse_ec);

  void ExtractHostPattern(const std::string& string, std::string* p_host,
                          std::string* p_pattern,
                          boost::system::error_code& ec) const;

 private:
  std::string input_pattern_;
  std::string output_pattern_;
  bool from_client_to_server_;
  bool stdin_input_;
  bool resume_;
  bool recursive_;
  bool check_file_integrity_;
  uint32_t max_parallel_copies_;
};

}  // command_line
}  // ssf

#endif  // SSF_CORE_COMMAND_LINE_COPY_COMMAND_LINE_H_
