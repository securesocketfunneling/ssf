#ifndef SSF_CORE_TRANSPORT_VIRTUAL_LAYER_POLICIES_INIT_PACKETS_SSF_REQUEST_H_
#define SSF_CORE_TRANSPORT_VIRTUAL_LAYER_POLICIES_INIT_PACKETS_SSF_REQUEST_H_

#include <cstdint>
#include <array>
#include <memory>

#include <boost/asio/buffer.hpp>

namespace ssf { 

class SSFRequest {
 public:
  typedef uint32_t version_field_type;

 enum { 
   field_number = 1, 
   total_size = sizeof(version_field_type)
 };

public:
  SSFRequest();
  SSFRequest(version_field_type version);

  version_field_type version() const;

  std::array<boost::asio::const_buffer, field_number> const_buffer() const;
  std::array<boost::asio::mutable_buffer, field_number> buffer();

private:
  version_field_type version_;
};

typedef std::shared_ptr<SSFRequest> SSFRequestPtr;

}  // ssf

#endif  // SSF_CORE_TRANSPORT_VIRTUAL_LAYER_POLICIES_INIT_PACKETS_SSF_REQUEST_H_
