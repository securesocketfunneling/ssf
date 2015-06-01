//
// fiber/detail/fiber_header.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2014-2015
//

#ifndef SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_FIBER_HEADER_HPP_
#define SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_FIBER_HEADER_HPP_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cstdint>
#include <vector>

#include "common/boost/fiber/detail/fiber_id.hpp"

namespace boost {
namespace asio {
namespace fiber {
namespace detail {

/// Class handling headers for fibers
class fiber_header
{
public:
  /// Type of the version field in the header
  typedef uint8_t version_type;

  /// Type for the raw header
  typedef fiber_id::raw_fiber_id raw_fiber_id;

  /// Type of the flags field in the header
  typedef uint8_t flags_type;

  /// Type of the data size field in the header
  typedef uint16_t data_size_type;

public:
  /// Constants used with the class
  enum { fiber_verion = 1, field_number = 3 + fiber_id::field_number };

public:
#pragma pack(push)
#pragma pack(1)
  /// Raw data structure to contain a fiber header
  struct raw_fiber_header
  {
    /// Version field
    version_type version;

    /// Raw id field
    raw_fiber_id fiber_id;

    /// Flags field
    flags_type flags;

    /// Data size field
    data_size_type data_size;
  };
#pragma pack(pop)

  /// Get a raw header
  raw_fiber_header get_raw()
  {
    raw_fiber_header raw;

    raw.version = version_;
    raw.fiber_id = id_.get_raw();
    raw.flags = flags_;
    raw.data_size = data_size_;

    return raw;
  }

public:
  /// Default constructor
  fiber_header()
    : version_(fiber_verion), id_(0, 0), flags_(0), data_size_(0)
  {
  }

  /// Constructor with each port specified
  /**
  * @param local_port The local port field to be set in the id
  * @param remote_port The remote port field to be set in the id
  * @param flags The flags to set in the header
  * @param data_size The data size to set in the header
  */
  fiber_header(fiber_id::local_port_type local_port,
                fiber_id::remote_port_type remote_port, flags_type flags,
                data_size_type data_size)
                : version_(fiber_verion),
                id_(remote_port, local_port),
                flags_(flags),
                data_size_(data_size)
  {
  }

  /// Constructor with a fiber ID specified
  /**
  * @param id the fiber id to set in the header
  * @param flags The flags to set in the header
  * @param data_size The data size to set in the header
  */
  fiber_header(fiber_id id, flags_type flags, data_size_type data_size)
    : version_(fiber_verion), id_(id), flags_(flags), data_size_(data_size)
  {
  }

  /// Constructor with a fiber ID and no data size
  /**
  * @param id the fiber id to set in the header
  * @param flags The flags to set in the header
  */
  fiber_header(fiber_id id, flags_type flags)
    : version_(fiber_verion), id_(id), flags_(flags), data_size_(0)
  {
  }

  /// Get the version
  version_type version() const { return version_; }

  /// Get the id
  fiber_id id() const { return id_; }

  /// Get the flags
  flags_type flags() const { return flags_; }

  /// Get the data size
  data_size_type data_size() const { return data_size_; }

  /// Set the id
  /**
  * @param id the fiber id to set in the header
  */
  void set_id(const fiber_id& id) { id_ = id; }

  /// Set the data size
  /**
  * @param data_size The data size to set in the header
  */
  void set_data_size(data_size_type data_size) { data_size_ = data_size; }

  /// Return the size of the header
  static uint16_t pod_size()
  {
    return sizeof(version_type)+fiber_id::pod_size() + sizeof(flags_type)+
      sizeof(data_size_type);
  }

  /// Get a buffer to receive a header
  std::vector<boost::asio::mutable_buffer> buffer()
  {
    std::vector<boost::asio::mutable_buffer> buf;

    buf.push_back(boost::asio::buffer(&version_, sizeof(version_)));
    id_.fill_buffer(buf);
    buf.push_back(boost::asio::buffer(&flags_, sizeof(flags_)));
    buf.push_back(boost::asio::buffer(&data_size_, sizeof(data_size_)));

    return buf;
  }

  /// Get a buffer to send a header
  std::vector<boost::asio::const_buffer> const_buffer()
  {
    std::vector<boost::asio::const_buffer> buf;

    buf.push_back(boost::asio::buffer(&version_, sizeof(version_)));
    id_.fill_const_buffer(buf);
    buf.push_back(boost::asio::buffer(&flags_, sizeof(flags_)));
    buf.push_back(boost::asio::buffer(&data_size_, sizeof(data_size_)));

    return buf;
  }

  /// Fill the given buffer vector with the fiber header fields
  /**
  * @param buffers The vector of buffers to fill
  */
  void fill_buffer(std::vector<boost::asio::mutable_buffer>& buffers)
  {
    buffers.push_back(boost::asio::buffer(&version_, sizeof(version_)));
    id_.fill_buffer(buffers);
    buffers.push_back(boost::asio::buffer(&flags_, sizeof(flags_)));
    buffers.push_back(boost::asio::buffer(&data_size_, sizeof(data_size_)));
  }

  /// Fill the given buffer vector with the fiber header fields
  /**
  * @param buffers The vector of buffers to fill
  */
  void fill_const_buffer(std::vector<boost::asio::const_buffer>& buffers)
  {
    buffers.push_back(boost::asio::buffer(&version_, sizeof(version_)));
    id_.fill_const_buffer(buffers);
    buffers.push_back(boost::asio::buffer(&flags_, sizeof(flags_)));
    buffers.push_back(boost::asio::buffer(&data_size_, sizeof(data_size_)));
  }

private:
  version_type version_;
  fiber_id id_;
  flags_type flags_;
  data_size_type data_size_;
};

} // namespace detail
} // namespace fiber
} // namespace asio
} // namespace boost

#endif  // SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_FIBER_HEADER_HPP_
