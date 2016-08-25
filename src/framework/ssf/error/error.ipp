#ifndef SSF_ERROR_ERROR_IPP_
#define SSF_ERROR_ERROR_IPP_

#include <string>

#include <boost/asio/error.hpp>

#include "ssf/error/error.h"

namespace ssf {
namespace error {

namespace detail {
class ssf_category : public boost::system::error_category {
 public:
  const char* name() const BOOST_ASIO_ERROR_CATEGORY_NOEXCEPT {
    return "ssf";
  }

  std::string message(int value) const {
    switch (value) {
      case error::success:
        return "success";
      case error::io_error:
        return "io_error";
      case error::interrupted:
        return "connection interrupted";
      case error::bad_file_descriptor:
        return "bad file descriptor";
      case error::device_or_resource_busy:
        return "device or resource busy";
      case error::invalid_argument:
        return "invalid argument";
      case error::not_a_socket:
        return "no socket could be created";
      case error::broken_pipe:
        return "broken pipe";
      case error::filename_too_long:
        return "filename too long";
      case error::function_not_supported:
        return "function not supported";
      case error::connection_aborted:
        return "connection aborted";
      case error::connection_refused:
        return "connection refused";
      case error::connection_reset:
        return "connection reset";
      case error::not_connected:
        return "not connected";
      case error::protocol_error:
        return "protocol error";
      case error::wrong_protocol_type:
        return "wrong protocol type";
      case error::operation_canceled:
        return "operation canceled";
      case error::identifier_removed:
        return "identifier removed";
      case error::address_in_use:
        return "address in use";
      case error::address_not_available:
        return "address not available";
      case error::bad_address:
        return "bad address";
      case error::message_size:
        return "message size";
      case error::network_down:
        return "network down";
      case error::no_buffer_space:
        return "no buffer space";
      case error::no_link:
        return "no link";
      case error::service_not_found:
        return "service not found";
      case error::out_of_range:
        return "out of range";
      case error::import_crt_error:
        return "could not import certificate";
      case error::set_crt_error:
        return "could not use certificate";
      case error::no_crt_error:
        return "no certificate found";
      case error::import_key_error:
        return "could not import key";
      case error::set_key_error:
        return "could not use key";
      case error::no_key_error:
        return "no key found";
      case error::no_dh_param_error:
        return "no dh parameter found";
      case error::buffer_is_full_error:
        return "buffer is full";
      case error::missing_config_parameters:
        return "missing config parameters";
      case error::cannot_resolve_endpoint:
        return "cannot resolve endpoint";
      default:
        return "ssf error";
    }
  }
};
}  // detail

inline const boost::system::error_category& get_ssf_category() {
  static detail::ssf_category instance;
  return instance;
}

}  // error
}  // ssf

#endif  // SSF_COMMON_ERROR_ERROR_IPP_
