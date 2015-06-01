//
// fiber/detail/fiber_buffer.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2014-2015
//

#ifndef SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_FIBER_BUFFER_HPP_
#define SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_FIBER_BUFFER_HPP_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/coroutine.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <vector>

#include "common/boost/fiber/detail/fiber_header.hpp"
#include "common/boost/fiber/detail/fiber_id.hpp"

namespace boost {
namespace asio {
namespace fiber {
namespace detail {

class fiber_buffer
{
public:
  /// Constructor for an empty fiber buffer of size 4096 bytes.
  fiber_buffer() : data_(4096), data_bytes_transferred_(0) {}

  /// Get the header part of the fiber buffer in order to fill it.
  fiber_header& header()
  {
    return header_;
  };

  /// Get the header part of the fiber buffer.
  const fiber_header& header() const
  {
    return header_;
  };

  /// Get the fiber id in the header part of the fiber buffer.
  const fiber_id fib_id() const { return header_.id(); }

  /// Set the header part of the fiber buffer.
  /**
  * @param header The header to put in the fiber buffer
  */
  void set_header(const fiber_header& header) { header_ = header; }

  /// Get the data part of the fiber buffer.
  const std::vector<uint8_t>& cdata() const { return data_; }

  /// Get the data part of the fiber buffer to fill it.
  std::vector<uint8_t>& data() { return data_; }

  /// Move the received data out of the fiber buffer.
  std::vector<uint8_t> take_data() { return std::move(data_); }

  /// Set the size of the received payload.
  /**
  * @param effective_bytes_transferred The size of the received payload
  */
  void set_bytes_transferred(std::size_t effective_bytes_transferred)
  {
    data_bytes_transferred_ = effective_bytes_transferred;
  }

  /// Get the size of the payload received
  std::size_t data_size() const { return data_bytes_transferred_; }

  /// Set the fiber id in the header part of the fiber buffer.
  /**
  * @param id The fiber id to set
  */
  void set_id(const fiber_id& id) { header_.set_id(id); }

  /// Increase the size of the current buffer
  /**
  * @param new_size The new_size of the buffer
  * (it has to be superior to the current size)
  */
  void resize(std::size_t new_size)
  {
    assert(new_size > data_.size());
    data_.resize(new_size);
  }

  /// Get the fiber buffer buffers for sending
  /**
  * @param buffers The buffers containing the payload for the fiber buffer.
  */
  template <typename ConstBufferSequence>
  std::vector<boost::asio::const_buffer> const_buffer(
    ConstBufferSequence buffers)
  {
    std::vector<boost::asio::const_buffer> buf;

    raw_ = header_.get_raw();
    buf.push_back(boost::asio::buffer((void*)&raw_, sizeof(raw_)));

    // header_.fill_const_buffer(buf);

    for (auto itor = buffers.begin(); itor != buffers.end(); ++itor)
    {
      buf.push_back(*itor);
    }

    return buf;
  }

  /// Get the fiber buffer buffer sequence for receiving.
  std::vector<boost::asio::mutable_buffer> buffer()
  {
    std::vector<boost::asio::mutable_buffer> buf;

    header_.fill_buffer(buf);
    buf.push_back(boost::asio::buffer(&data_, data_.size()));

    return buf;
  }

private:
  fiber_header header_;
  std::vector<uint8_t> data_;

  std::size_t data_bytes_transferred_;

  fiber_header::raw_fiber_header raw_;
};

typedef std::shared_ptr<fiber_buffer> p_fiber_buffer;

/// Coroutine class to receive a fiber buffer from a stream
/**
* @tparam StreamSocket The stream type to receive from
* @tparam ReadHandler The type for the callback handler to call upon reception
*/
template <typename StreamSocket, typename ReadHandler>
class read_fiber_buf_op : public boost::asio::coroutine
{
public:
  /// Constructor
  /**
  * @param stream The stream to receive from
  * @param buffer The fiber buffer to receive into
  * @param handler The handler to be called upon reception
  */
  read_fiber_buf_op(StreamSocket& stream, fiber_buffer& buffer,
                    ReadHandler& handler)
                    : stream_(stream),
                    buffer_(buffer),
                    handler_(handler),
                    total_transferred_(0),
                    data_transferred_(0)
  {
  }

#include <boost/asio/yield.hpp>  // NOLINT
  void operator()(const boost::system::error_code& ec, std::size_t length)
  {
    auto& header = buffer_.header();

    if (!ec) reenter(this)
    {
      // Receive the header
      yield boost::asio::async_read(stream_, header.buffer(),
                                    std::move(*this));
      total_transferred_ += length;

      // Resize the buffer if needed
      if (header.data_size() > buffer_.cdata().size())
      {
        buffer_.resize(header.data_size());
      }

      // Receive the payload
      yield boost::asio::async_read(
        stream_, boost::asio::buffer(buffer_.data(), header.data_size()),
        std::move(*this));

      data_transferred_ = length;
      total_transferred_ += data_transferred_;
      buffer_.set_bytes_transferred(data_transferred_);
      buffer_.set_id(header.id());

      handler_(ec, total_transferred_);
    }
    else
    {
      handler_(ec, total_transferred_);
    }
  }
#include <boost/asio/unyield.hpp>  // NOLINT

private:
  StreamSocket& stream_;
  fiber_buffer& buffer_;
  ReadHandler handler_;
  std::size_t total_transferred_;
  std::size_t data_transferred_;
};

/// Helper to receive a fiber packet in a fiber buffer
/**
* @tparam StreamSocket The stream type to receive from
* @tparam ReadHandler The type for the callback handler to call upon reception
*
* @param stream The stream to receive from
* @param buffer The fiber buffer to receive into
* @param handler The handler to be called upon reception
*/
template <typename StreamSocket, typename ReadHandler>
void async_read_fiber_buffer(StreamSocket& stream, fiber_buffer& buffer,
                              ReadHandler& handler)
{
  // Start the coroutine
  read_fiber_buf_op<StreamSocket, ReadHandler>(stream, buffer, handler)(
    boost::system::error_code(), 0);
}

} // namespace detail
} // namespace fiber
} // namespace asio
} // namespace boost

#endif  // SSF_COMMON_BOOST_ASIO_FIBER_DETAIL_FIBER_BUFFER_HPP_
