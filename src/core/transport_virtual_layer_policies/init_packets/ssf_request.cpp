#include "ssf_request.h"

namespace ssf {

SSFRequest::SSFRequest() : version_(0) {}

SSFRequest::SSFRequest(VersionField version) : version_(version) {}

SSFRequest::VersionField SSFRequest::version() const { return version_; }

std::array<boost::asio::const_buffer, SSFRequest::field_number>
SSFRequest::const_buffer() const {
  std::array<boost::asio::const_buffer, field_number> buf = {
      {boost::asio::const_buffer(&version_, sizeof(version_))}};

  return buf;
}

std::array<boost::asio::mutable_buffer, SSFRequest::field_number>
SSFRequest::buffer() {
  std::array<boost::asio::mutable_buffer, field_number> buf = {
      {boost::asio::mutable_buffer(&version_, sizeof(version_))}};

  return buf;
}

}  // ssf
