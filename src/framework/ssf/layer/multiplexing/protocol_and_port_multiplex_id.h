#ifndef SSF_LAYER_MULTIPLEXING_PROTOCOL_N_PORT_MULTIPLEX_ID_H_
#define SSF_LAYER_MULTIPLEXING_PROTOCOL_N_PORT_MULTIPLEX_ID_H_

#include <cstdint>

#include <set>
#include <string>
#include <vector>

#include <boost/asio/io_service.hpp>
#include <boost/asio/buffer.hpp>

#include "common/utils/map_helpers.h"

#include "ssf/layer/parameters.h"

namespace ssf {
namespace layer {
namespace multiplexing {

class ProtocolAndPortID {
 private:
  typedef uint8_t ProtocolID;
  typedef uint16_t PortID;

 public:
  typedef io::fixed_const_buffer_sequence ConstBuffers;
  typedef io::fixed_mutable_buffer_sequence MutableBuffers;
  enum { size = sizeof(ProtocolID) + sizeof(PortID) };

 public:
  ProtocolAndPortID() : protocol_id_(0), port_id_(0) {}
  ProtocolAndPortID(ProtocolID protocol_id, PortID port_id)
      : protocol_id_(protocol_id), port_id_(port_id) {}
  ~ProtocolAndPortID() {}

  bool operator==(const ProtocolAndPortID& rhs) const {
    return ((protocol_id_ == rhs.protocol_id_) && (port_id_ == rhs.port_id_));
  }

  bool operator!=(const ProtocolAndPortID& rhs) const {
    return !(*this == rhs);
  }

  bool operator<(const ProtocolAndPortID& rhs) const {
    return (protocol_id_ < rhs.protocol_id_) ||
           ((protocol_id_ == rhs.protocol_id_) && (port_id_ < rhs.port_id_));
  }

  bool operator!() const { return !(!!protocol_id_ && !!port_id_); }

  ConstBuffers GetConstBuffers() const {
    ConstBuffers result;
    result.push_back(boost::asio::buffer(&protocol_id_, sizeof(protocol_id_)));
    result.push_back(boost::asio::buffer(&port_id_, sizeof(port_id_)));
    return result;
  }

  void GetConstBuffers(ConstBuffers* p_buffers) const {
    p_buffers->push_back(
        boost::asio::buffer(&protocol_id_, sizeof(protocol_id_)));
    p_buffers->push_back(boost::asio::buffer(&port_id_, sizeof(port_id_)));
  }

  MutableBuffers GetMutableBuffers() {
    MutableBuffers result;
    result.push_back(boost::asio::buffer(&protocol_id_, sizeof(protocol_id_)));
    result.push_back(boost::asio::buffer(&port_id_, sizeof(port_id_)));
    return result;
  }

  void GetMutableBuffers(MutableBuffers* p_buffers) {
    p_buffers->push_back(
        boost::asio::buffer(&protocol_id_, sizeof(protocol_id_)));
    p_buffers->push_back(boost::asio::buffer(&port_id_, sizeof(port_id_)));
  }

  ProtocolID& protocol_id() { return protocol_id_; }
  const ProtocolID& protocol_id() const { return protocol_id_; }

  PortID& port_id() { return port_id_; }
  const PortID& port_id() const { return port_id_; }

 private:
  ProtocolID protocol_id_;
  PortID port_id_;
};

class ProtocolAndPortPairID {
 private:
  typedef ProtocolAndPortID ID;

 public:
  typedef io::fixed_const_buffer_sequence ConstBuffers;
  typedef io::fixed_mutable_buffer_sequence MutableBuffers;
  enum { size = 2 * ID::size };

 public:
  ProtocolAndPortPairID() : left_id_(), right_id_() {}
  ProtocolAndPortPairID(ID l_id, ID r_id) : left_id_(l_id), right_id_(r_id) {}
  ~ProtocolAndPortPairID() {}

  bool operator==(const ProtocolAndPortPairID& rhs) const {
    return (left_id_ == rhs.left_id_) && (right_id_ == rhs.right_id_);
  }

  bool operator!=(const ProtocolAndPortPairID& rhs) const {
    return !(rhs == *this);
  }

  bool operator<(const ProtocolAndPortPairID& rhs) const {
    return (left_id_ < rhs.left_id_) ||
           ((left_id_ == rhs.left_id_) && (right_id_ < rhs.right_id_));
  }

  ConstBuffers GetConstBuffers() const {
    ConstBuffers result;
    left_id_.GetConstBuffers(&result);
    right_id_.GetConstBuffers(&result);
    return result;
  }

  void GetConstBuffers(ConstBuffers* p_buffers) const {
    left_id_.GetConstBuffers(p_buffers);
    right_id_.GetConstBuffers(p_buffers);
  }

  MutableBuffers GetMutableBuffers() {
    MutableBuffers result;
    left_id_.GetMutableBuffers(&result);
    right_id_.GetMutableBuffers(&result);
    return result;
  }

  void GetMutableBuffers(MutableBuffers* p_buffers) {
    left_id_.GetMutableBuffers(p_buffers);
    right_id_.GetMutableBuffers(p_buffers);
  }

  ID& left_id() { return left_id_; }
  const ID& left_id() const { return left_id_; }

  ID& right_id() { return right_id_; }
  const ID& right_id() const { return right_id_; }

  ID GetFirstHalfId() const { return left_id_; }
  ID GetSecondHalfId() const { return right_id_; }

 private:
  ID left_id_;
  ID right_id_;
};

class ProtocolAndPortMultiplexID {
 public:
  typedef ProtocolAndPortID HalfID;
  typedef ProtocolAndPortPairID FullID;
  enum { size = FullID::size };

 public:
  static HalfID MakeHalfRemoteID(FullID full_id) {
    return full_id.GetFirstHalfId();
  }

  static HalfID MakeHalfID(boost::asio::io_service& io_service,
                           const LayerParameters& parameters, uint32_t id) {
    auto protocol_string =
        helpers::GetField<std::string>("protocol", parameters);
    auto port_string = helpers::GetField<std::string>("port", parameters);

    try {
      return HalfID((uint8_t)std::stoul(protocol_string),
                    (uint16_t)std::stoul(port_string));
    }
    catch (const std::exception&) {
      return HalfID();
    }
  }

  static HalfID LocalHalfIDFromRemote(const HalfID& remote_id,
                                      const std::set<HalfID>& used_local_ids) {
    for (uint16_t i = 1; i < std::numeric_limits<uint16_t>::max(); ++i) {
      auto half_id = HalfID(remote_id.protocol_id(), i);
      if (!used_local_ids.count(half_id)) {
        return half_id;
      }
    }

    return HalfID();
  }
};

}  // multiplexing
}  // layer
}  // ssf

#endif  // SSF_LAYER_MULTIPLEXING_PROTOCOL_N_PORT_MULTIPLEX_ID_H_
