#ifndef SSF_CORE_PARSER_BOUNCE_PARSER_H_
#define SSF_CORE_PARSER_BOUNCE_PARSER_H_

#include <list>
#include <string>

namespace ssf {
namespace parser {

class BounceParser {
 public:
  using BounceList = std::list<std::string>;

 public:
  static BounceList ParseBounceFile(const std::string& filepath);

  static std::string GetRemoteAddress(const std::string& bounce_line);

  static std::string GetRemotePort(const std::string& bounce_line);
};

}  // parser
}  // ssf

#endif  // SSF_CORE_PARSER_BOUNCE_PARSER_H_
