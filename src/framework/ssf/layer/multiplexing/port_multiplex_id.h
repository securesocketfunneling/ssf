#ifndef SSF_LAYER_MULTIPLEXING_PORT_MULTIPLEX_ID_H_
#define SSF_LAYER_MULTIPLEXING_PORT_MULTIPLEX_ID_H_

#include <cstdint>

#include <vector>

#include <boost/asio/buffer.hpp>

#include "ssf/utils/map_helpers.h"

#include "ssf/layer/parameters.h"

namespace ssf {
namespace layer {
namespace multiplexing {

class PortID {
 private:
  typedef uint16_t ID;

 public:
  typedef io::fixed_const_buffer_sequence ConstBuffers;
  typedef io::fixed_mutable_buffer_sequence MutableBuffers;
  enum { size = sizeof(ID) };

 public:
  PortID() : id_(0) {}
  PortID(ID id) : id_(id) {}
  ~PortID() {}

  bool operator==(const PortID& rhs) const { return id_ == rhs.id_; }
  bool operator!=(const PortID& rhs) const { return id_ != rhs.id_; }
  bool operator<(const PortID& rhs) const { return id_ < rhs.id_; }
  bool operator!() const { return !id_; }

  ConstBuffers GetConstBuffers() const {
    return ConstBuffers({boost::asio::buffer(&id_, sizeof(id_))});
  }

  void GetConstBuffers(ConstBuffers* p_buffers) const {
    p_buffers->push_back(boost::asio::buffer(&id_, sizeof(id_)));
  }

  MutableBuffers GetMutableBuffers() {
    return MutableBuffers({boost::asio::buffer(&id_, sizeof(id_))});
  }

  void GetMutableBuffers(MutableBuffers* p_buffers) {
    p_buffers->push_back(boost::asio::buffer(&id_, sizeof(id_)));
  }

  ID& id() { return id_; }
  const ID& id() const { return id_; }

 private:
  ID id_;
};

class PortPairID {
 private:
  typedef PortID ID;

 public:
  typedef io::fixed_const_buffer_sequence ConstBuffers;
  typedef io::fixed_mutable_buffer_sequence MutableBuffers;
  enum { size = 2 * ID::size };

 public:
  PortPairID() : left_id_(0), right_id_(0) {}
  PortPairID(ID l_id, ID r_id) : left_id_(l_id), right_id_(r_id) {}
  ~PortPairID() {}

  bool operator==(const PortPairID& rhs) const {
    return (left_id_ == rhs.left_id_) && (right_id_ == rhs.right_id_);
  }

  bool operator!=(const PortPairID& rhs) const { return !(rhs == *this); }

  bool operator<(const PortPairID& rhs) const {
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

class PortMultiplexID {
 public:
  typedef PortID HalfID;
  typedef PortPairID FullID;
  enum { size = FullID::size };

 public:
  static HalfID MakeHalfRemoteID(FullID full_id) {
    return full_id.GetFirstHalfId();
  }

  static HalfID MakeHalfID(boost::asio::io_service& io_service,
                           const LayerParameters& parameters, uint32_t id) {
    auto id_string = helpers::GetField<std::string>("port", parameters);

    try {
      return HalfID((uint16_t)std::stoul(id_string));
    }
    catch (const std::exception&) {
      return HalfID();
    }
  }

  static HalfID LocalHalfIDFromRemote(const HalfID& remote_id,
                                      const std::set<HalfID>& used_local_ids) {
    for (uint16_t i = 1; i < std::numeric_limits<uint16_t>::max(); ++i) {
      auto half_id = HalfID(i);
      if (!used_local_ids.count(half_id)) {
        return half_id;
      }
    }

    return HalfID(0);
  }
};

}  // multiplexing
}  // layer
}  // ssf

#endif  // SSF_LAYER_MULTIPLEXING_PORT_MULTIPLEX_ID_H_
