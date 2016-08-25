#ifndef SSF_CORE_TRANSPORT_VIRTUAL_LAYER_POLICIES_INIT_PACKETS_SSF_REPLY_H_
#define SSF_CORE_TRANSPORT_VIRTUAL_LAYER_POLICIES_INIT_PACKETS_SSF_REPLY_H_

#include <cstdint>
#include <array>
#include <memory>

#include <boost/asio/buffer.hpp>

namespace ssf {

class SSFReply {
 public:
  enum { field_number = 1, total_size = sizeof(bool) };

 public:
  SSFReply();
  SSFReply(bool result);

  bool result() const;

  std::array<boost::asio::const_buffer, field_number> const_buffer() const;

  std::array<boost::asio::mutable_buffer, field_number> buffer();

 private:
  bool result_;
};

using SSFReplyPtr = std::shared_ptr<SSFReply>;

}  // ssf

#endif  // SSF_CORE_TRANSPORT_VIRTUAL_LAYER_POLICIES_INIT_PACKETS_SSF_REPLY_H_
