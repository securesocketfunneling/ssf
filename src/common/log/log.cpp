#include <ssf/log/log.h>

#include "common/log/log.h"

namespace ssf {
namespace log {

void Configure() {
  // TODO : improve with severity level param
  ssf::log::Log::SetSeverityLevel(ssf::log::kLogInfo);
}

}  // log
}  // ssf