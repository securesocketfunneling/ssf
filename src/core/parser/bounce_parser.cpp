#include "core/parser/bounce_parser.h"

#include <fstream>

namespace ssf {
namespace parser {

BounceParser::BounceList BounceParser::ParseBounceFile(
    const std::string& filepath) {
  BounceList list;

  if (filepath != "") {
    std::ifstream file(filepath);

    if (file.is_open()) {
      std::string line;

      while (std::getline(file, line)) {
        list.push_back(line);
      }

      file.close();
    }
  }

  return list;
}

std::string BounceParser::GetRemoteAddress(const std::string& bounce_line) {
  size_t position = bounce_line.find(":");

  if (position != std::string::npos) {
    return bounce_line.substr(0, position);
  } else {
    return "";
  }
}

std::string BounceParser::GetRemotePort(const std::string& bounce_line) {
  size_t position = bounce_line.find(":");

  if (position != std::string::npos) {
    return bounce_line.substr(position + 1);
  } else {
    return "";
  }
}

}  // parser
}  // ssf