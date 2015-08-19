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
  if (value == error::io_error)
    return "io_error";
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
  if (value == error::function_not_supported)
    return "function not supported";
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
  if (value == error::identifier_removed)
    return "identifier removed";
  if (value == error::address_in_use)
    return "address in use";
  if (value == error::address_not_available)
    return "address not available";
  if (value == error::bad_address)
    return "bad address";
  if (value == error::message_size)
    return "message size";
  if (value == error::network_down)
    return "network down";
  if (value == error::no_buffer_space)
    return "no buffer space";
  if (value == error::no_link)
    return "no link";
  if (value == error::service_not_found)
    return "service not found";
  if (value == error::out_of_range)
    return "out of range";
  if (value == error::import_crt_error)
    return "could not import certificate";
  if (value == error::set_crt_error)
    return "could not use certificate";
  if (value == error::no_crt_error)
    return "no certificate found";
  if (value == error::import_key_error)
    return "could not import key";
  if (value == error::set_key_error)
    return "could not use key";
  if (value == error::no_key_error)
    return "no key found";
  if (value == error::no_dh_param_error)
    return "no dh parameter found";
  if (value == error::buffer_is_full_error)
    return "buffer is full";
  if (value == error::missing_config_parameters)
    return "missing config parameters";
  if (value == error::cannot_resolve_endpoint)
    return "cannot resolve endpoint";
  return "ssf error";
}
