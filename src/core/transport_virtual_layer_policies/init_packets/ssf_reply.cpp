#include "ssf_reply.h"

namespace ssf {

SSFReply::SSFReply() : result_(false) {}
SSFReply::SSFReply(bool result) : result_(result) {}

bool SSFReply::result() const { return result_; }

std::array<boost::asio::const_buffer, SSFReply::field_number>
SSFReply::const_buffer() const {
  std::array<boost::asio::const_buffer, field_number> buf = {
      {boost::asio::const_buffer(&result_, sizeof(result_))}};

  return buf;
}

std::array<boost::asio::mutable_buffer, SSFReply::field_number>
SSFReply::buffer() {
  std::array<boost::asio::mutable_buffer, field_number> buf = {
      {boost::asio::mutable_buffer(&result_, sizeof(result_))}};

  return buf;
}

}  // ssf
