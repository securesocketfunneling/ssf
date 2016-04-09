#ifndef SSF_COMMON_ERROR_ERROR_H_
#define SSF_COMMON_ERROR_ERROR_H_

#include <string>

#include <boost/asio/error.hpp>
#include <boost/system/error_code.hpp>

namespace error {

enum errors {
  success = boost::system::errc::success,
  interrupted = boost::system::errc::interrupted,
  bad_file_descriptor = boost::system::errc::bad_file_descriptor,
  device_or_resource_busy = boost::system::errc::device_or_resource_busy,
  invalid_argument = boost::system::errc::invalid_argument,
  not_a_socket = boost::system::errc::not_a_socket,
  broken_pipe = boost::system::errc::broken_pipe,
  filename_too_long = boost::system::errc::filename_too_long,
  message_too_long = boost::asio::error::basic_errors::message_size,
  connection_aborted = boost::system::errc::connection_aborted,
  connection_refused = boost::system::errc::connection_refused,
  connection_reset = boost::system::errc::connection_reset,
  not_connected = boost::system::errc::not_connected,
  protocol_error = boost::system::errc::protocol_error,
  wrong_protocol_type = boost::system::errc::wrong_protocol_type,
  operation_canceled = boost::system::errc::operation_canceled,
  service_not_found = 10000,
  service_not_started = 10001,
  out_of_range = 10002
};

namespace detail {
class ssf_category : public boost::system::error_category {
 public:
  const char* name() const BOOST_SYSTEM_NOEXCEPT;

  std::string message(int value) const;
};
}  // detail

inline const boost::system::error_category& get_ssf_category() {
  static detail::ssf_category instance;
  return instance;
}

}  // error

#endif  // SSF_COMMON_ERROR_ERROR_H_
