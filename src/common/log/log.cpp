#include "common/log/log.h"

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

namespace ssf {
namespace log {

void Configure() {
  // TODO : improve with severity level param
  namespace logging = boost::log;

  logging::add_common_attributes();
  auto formatTimestamp =
      logging::expressions::format_date_time<boost::posix_time::ptime>(
          "TimeStamp", "%Y-%m-%d %H:%M:%S");
  auto formatSeverity =
      logging::expressions::attr<logging::trivial::severity_level>("Severity");
  logging::formatter logFormatter =
      logging::expressions::format("[%1%][%2%] %3%") % formatSeverity %
      formatTimestamp % logging::expressions::smessage;
  auto p_sink = logging::add_console_log(std::clog);
  p_sink->set_formatter(logFormatter);

  logging::core::get()->set_filter(logging::trivial::severity >=
                                   logging::trivial::info);
}

}  // log
}  // ssf