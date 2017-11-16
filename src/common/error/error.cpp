#include "common/error/error.h"

const char* error::detail::ssf_category::name() const BOOST_SYSTEM_NOEXCEPT {
  return "error";
}

std::string error::detail::ssf_category::message(int value) const {
  switch (value) {
    case error::success:
      return "success";
      break;
    case error::interrupted:
      return "connection interrupted";
      break;
    case error::bad_file_descriptor:
      return "bad file descriptor";
      break;
    case error::device_or_resource_busy:
      return "device or resource busy";
      break;
    case error::invalid_argument:
      return "invalid argument";
      break;
    case error::not_a_socket:
      return "no socket could be created";
      break;
    case error::broken_pipe:
      return "broken pipe";
      break;
    case error::filename_too_long:
      return "filename too long";
      break;
    case error::message_too_long:
      return "message too long";
      break;
    case error::connection_aborted:
      return "connection aborted";
      break;
    case error::connection_refused:
      return "connection refused";
      break;
    case error::connection_reset:
      return "connection reset";
      break;
    case error::address_not_available:
      return "address not available";
      break;
    case error::destination_address_required:
      return "destination address required";
      break;
    case error::host_unreachable:
      return "host unreachable";
      break;
    case error::not_connected:
      return "not connected";
      break;
    case error::protocol_error:
      return "protocol error";
    case error::protocol_not_supported:
      return "protocol not supported";
      break;
    case error::wrong_protocol_type:
      return "wrong protocol type";
      break;
    case error::operation_canceled:
      return "operation canceled";
      break;
    case error::operation_not_supported:
      return "operation not supported";
      break;
    case error::service_not_found:
      return "service not found";
      break;
    case error::service_not_started:
      return "service not started";
      break;
    case error::out_of_range:
      return "out of range";
      break;
    case error::process_not_created:
      return "process not created";
      break;
    case error::file_not_found:
      return "file not found";
      break;
    default:
      return "error";
  }
}