#ifndef SSF_CORE_COMMAND_LINE_COPY_COMMAND_LINE_H_
#define SSF_CORE_COMMAND_LINE_COPY_COMMAND_LINE_H_

#include <cstdint>

#include <string>
#include <stdexcept>
#include <vector>
#include <regex>
#include <memory>

#include <boost/program_options.hpp>
#include <boost/system/error_code.hpp>

namespace ssf {
namespace command_line {
namespace copy {

class CommandLine {
 public:
  CommandLine();

  void parse(int argc, char* argv[], boost::system::error_code& ec);

  uint16_t port() const;

  std::string addr() const;

  std::string bounce_file() const;

  std::string config_file() const;

  bool from_stdin() const;

  bool from_local_to_remote() const;

  std::string input_pattern() const;

  std::string output_pattern() const;

  bool IsPortSet() const;

  bool IsAddrSet() const;

 private:
  void InternalParsing(const boost::program_options::variables_map& vm,
                       boost::system::error_code& ec);

  void ParsePort(int port, boost::system::error_code& parse_ec);

  void ParseFirstArgument(const std::string& first_arg,
                          boost::system::error_code& parse_ec);

  void ParseSecondArgument(const std::string& second_arg,
                           boost::system::error_code& parse_ec);

  void ExtractHostPattern(const std::string& string, std::string* p_host,
                          std::string* p_pattern,
                          boost::system::error_code& ec) const;

  char GetHostDirectorySeparator() const;

 private:
  uint16_t port_;
  std::string addr_;
  std::string bounce_file_;
  std::string config_file_;
  std::string input_pattern_;
  std::string output_pattern_;
  bool from_stdin_;
  bool from_local_to_remote_;
  bool addr_set_;
  bool port_set_;
};

}  // copy
}  // command_line
}  // ssf

#endif  // SSF_CORE_COMMAND_LINE_COPY_COMMAND_LINE_H_
