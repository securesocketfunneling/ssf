#ifndef SSF_LAYER_NETWORK_NETWORK_ID_H_
#define SSF_LAYER_NETWORK_NETWORK_ID_H_

#include <cstdint>

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>

#include "ssf/io/buffers.h"
#include "ssf/utils/map_helpers.h"

#include "ssf/layer/parameters.h"

namespace ssf {
namespace layer {
namespace network {

class NetworkID {
 public:
  typedef uint16_t HalfID;

  typedef io::fixed_const_buffer_sequence ConstBuffers;
  typedef io::fixed_mutable_buffer_sequence MutableBuffers;
  enum { size = 2 * sizeof(HalfID) };

 public:
  NetworkID() : local_id_(0), remote_id_(0) {}

  NetworkID(HalfID local, HalfID remote)
      : local_id_(local), remote_id_(remote) {}

  ConstBuffers GetConstBuffers() const {
    ConstBuffers buffers;
    buffers.push_back(boost::asio::buffer(&local_id_, sizeof(HalfID)));
    buffers.push_back(boost::asio::buffer(&remote_id_, sizeof(HalfID)));
    return buffers;
  }

  void GetConstBuffers(ConstBuffers* p_buffers) const {
    p_buffers->push_back(boost::asio::buffer(&local_id_, sizeof(HalfID)));
    p_buffers->push_back(boost::asio::buffer(&remote_id_, sizeof(HalfID)));
  }

  MutableBuffers GetMutableBuffers() {
    MutableBuffers buffers;
    buffers.push_back(boost::asio::buffer(&local_id_, sizeof(HalfID)));
    buffers.push_back(boost::asio::buffer(&remote_id_, sizeof(HalfID)));
    return buffers;
  }

  void GetMutableBuffers(MutableBuffers* p_buffers) {
    p_buffers->push_back(boost::asio::buffer(&local_id_, sizeof(HalfID)));
    p_buffers->push_back(boost::asio::buffer(&remote_id_, sizeof(HalfID)));
  }

  HalfID& left_id() { return local_id_; }
  const HalfID& left_id() const { return local_id_; }

  HalfID& right_id() { return remote_id_; }
  const HalfID& right_id() const { return remote_id_; }

  HalfID GetFirstHalfId() const { return local_id_; }
  HalfID GetSecondHalfId() const { return remote_id_; }

  static HalfID MakeHalfID(boost::asio::io_service& io_service,
                           const LayerParameters& parameters, uint32_t) {
    auto id_string = helpers::GetField<std::string>("network_id", parameters);

    try {
      return HalfID(std::stoul(id_string));
    } catch (const std::exception&) {
      return 0;
    }
  }

 private:
  HalfID local_id_;
  HalfID remote_id_;
};

}  // network
}  // layer
}  // ssf

#endif  // SSF_LAYER_NETWORK_NETWORK_ID_H_
