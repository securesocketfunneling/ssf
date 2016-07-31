#include <ssf/log/log.h>

#include "common/log/log.h"

namespace ssf {
namespace log {

void Configure(LogLevel level) {
  ssf::log::Log::SetSeverityLevel(level);
}

}  // log
}  // ssf