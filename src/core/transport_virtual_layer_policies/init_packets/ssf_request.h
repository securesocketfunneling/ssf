#ifndef SSF_CORE_TRANSPORT_VIRTUAL_LAYER_POLICIES_INIT_PACKETS_SSF_REQUEST_H_
#define SSF_CORE_TRANSPORT_VIRTUAL_LAYER_POLICIES_INIT_PACKETS_SSF_REQUEST_H_

#include <cstdint>
#include <array>
#include <memory>

#include <boost/asio/buffer.hpp>

namespace ssf {

class SSFRequest {
 public:
  using VersionField = uint32_t;

  enum { field_number = 1, total_size = sizeof(VersionField) };

 public:
  SSFRequest();
  SSFRequest(VersionField version);

  VersionField version() const;

  std::array<boost::asio::const_buffer, field_number> const_buffer() const;
  std::array<boost::asio::mutable_buffer, field_number> buffer();

 private:
  VersionField version_;
};

using SSFRequestPtr = std::shared_ptr<SSFRequest>;

}  // ssf

#endif  // SSF_CORE_TRANSPORT_VIRTUAL_LAYER_POLICIES_INIT_PACKETS_SSF_REQUEST_H_
