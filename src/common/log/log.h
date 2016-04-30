#ifndef SSF_COMMON_LOG_LOG_H_
#define SSF_COMMON_LOG_LOG_H_

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>

namespace ssf {
namespace log {

void Configure();

}  // log
}  // ssf

#endif  // SSF_COMMON_LOG_LOG_H_
