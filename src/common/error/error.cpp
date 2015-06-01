#include "error.h"

#include <boost/system/error_code.hpp>
#include <boost/asio/error.hpp>
#include <string>

const char* ssf::error::detail::ssf_category::name() const BOOST_SYSTEM_NOEXCEPT
{
  return "ssf";
}

std::string ssf::error::detail::ssf_category::message(int value) const {
  if (value == error::success)
    return "success";
  if (value == error::interrupted)
    return "connection interrupted";
  if (value == error::bad_file_descriptor)
    return "bad file descriptor";
  if (value == error::device_or_resource_busy)
    return "device or resource busy";
  if (value == error::invalid_argument)
    return "invalid argument";
  if (value == error::not_a_socket)
    return "no socket could be created";
  if (value == error::broken_pipe)
    return "broken pipe";
  if (value == error::filename_too_long)
    return "filename too long";
  if (value == error::message_too_long)
    return "message too long";
  if (value == error::connection_aborted)
    return "connection aborted";
  if (value == error::connection_refused)
    return "connection refused";
  if (value == error::connection_reset)
    return "connection reset";
  if (value == error::not_connected)
    return "not connected";
  if (value == error::protocol_error)
    return "protocol error";
  if (value == error::wrong_protocol_type)
    return "wrong protocol type";
  if (value == error::operation_canceled)
    return "operation canceled";
  if (value == error::service_not_found)
    return "service not found";
  if (value == error::out_of_range)
    return "out of range";
  return "ssf error";
}
