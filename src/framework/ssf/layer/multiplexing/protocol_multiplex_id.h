#ifndef SSF_LAYER_MULTIPLEXING_PROTOCOL_MULTIPLEX_ID_H_
#define SSF_LAYER_MULTIPLEXING_PROTOCOL_MULTIPLEX_ID_H_

#include <cstdint>

#include <vector>

#include <boost/asio/buffer.hpp>

#include "ssf/utils/map_helpers.h"

#include "ssf/layer/parameters.h"

namespace ssf {
namespace layer {
namespace multiplexing {

class ProtocolID {
 private:
  typedef uint8_t ID;

 public:
  typedef io::fixed_const_buffer_sequence ConstBuffers;
  typedef io::fixed_mutable_buffer_sequence MutableBuffers;
  enum { size = sizeof(ID) };

 public:
  ProtocolID() : id_(0) {}
  ProtocolID(ID id) : id_(id) {}
  ProtocolID(ProtocolID id1, ProtocolID id2) : id_(id1.id_) {}
  ~ProtocolID() {}

  bool operator==(const ProtocolID& rhs) const { return id_ == rhs.id_; }
  bool operator!=(const ProtocolID& rhs) const { return id_ != rhs.id_; }
  bool operator<(const ProtocolID& rhs) const { return id_ < rhs.id_; }
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

  ID GetFirstHalfId() const { return id_; }
  ID GetSecondHalfId() const { return id_; }

 private:
  ID id_;
};

class ProtocolMultiplexID {
 public:
  typedef ProtocolID HalfID;
  typedef ProtocolID FullID;
  enum { size = ProtocolID::size };

 public:
  static HalfID MakeHalfRemoteID(FullID full_id) { return full_id; }

  static HalfID MakeHalfID(boost::asio::io_service& io_service,
                           const LayerParameters& parameters, uint32_t id) {
    if (!id) {
      auto id_string =
          helpers::GetField<std::string>("protocol_id", parameters);

      try {
        return HalfID((uint8_t)std::stoul(id_string));
      }
      catch (const std::exception&) {
        return HalfID();
      }
    } else {
      return HalfID((uint8_t)id);
    }
  }

  static HalfID LocalHalfIDFromRemote(const HalfID& remote_id,
                                      const std::set<HalfID>& used_local_ids) {
    return remote_id;
  }
};

}  // multiplexing
}  // layer
}  // ssf

#endif  // SSF_LAYER_MULTIPLEXING_PROTOCOL_MULTIPLEX_ID_H_
