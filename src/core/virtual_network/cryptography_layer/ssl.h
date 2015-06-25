#ifndef SSF_CORE_VIRTUAL_NETWORK_CRYPTOGRAPHY_LAYER_SSL_H_
#define SSF_CORE_VIRTUAL_NETWORK_CRYPTOGRAPHY_LAYER_SSL_H_

#include <cstdint>

#include <memory>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/system/error_code.hpp>
#include <boost/bind.hpp>
#include <boost/log/trivial.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/detail/handler_type_requirements.hpp>

#include "core/virtual_network/cryptography_layer/ssl_helpers.h"

#include "common/error/error.h"
#include "common/io/read_stream_op.h"

#include "core/virtual_network/basic_endpoint.h"
#include "core/virtual_network/protocol_attributes.h"

namespace virtual_network {
namespace cryptography_layer {

namespace detail {

/// The class in charge of receiving data from an ssl stream into a buffer
template <typename NextLayerStreamSocket>
class SSLStreamBufferer : public std::enable_shared_from_this<
                              SSLStreamBufferer<NextLayerStreamSocket>> {
 private:
  enum {
    lower_queue_size_bound = 1 * 1024 * 1024,
    higher_queue_size_bound = 16 * 1024 * 1024,
    receive_buffer_size = 50 * 1024
  };

 private:
  typedef boost::asio::ssl::stream<NextLayerStreamSocket> ssl_stream_type;
  typedef std::shared_ptr<ssl_stream_type> p_ssl_stream_type;
  typedef detail::ExtendedSSLContext p_context_type;
  typedef boost::asio::io_service::strand strand_type;
  typedef std::shared_ptr<strand_type> p_strand_type;

 public:
  typedef SSLStreamBufferer<NextLayerStreamSocket> puller_type;
  typedef std::shared_ptr<puller_type> p_puller_type;
  typedef boost::asio::detail::op_queue<io::basic_pending_read_stream_operation>
      op_queue_type;

 public:
  SSLStreamBufferer(const SSLStreamBufferer&) = delete;
  SSLStreamBufferer& operator=(const SSLStreamBufferer&) = delete;

  ~SSLStreamBufferer() {}

  static p_puller_type create(p_ssl_stream_type p_socket,
                              p_strand_type p_strand, p_context_type p_ctx) {
    return p_puller_type(new puller_type(p_socket, p_strand, p_ctx));
  }

  /// Start receiving data
  void start_pulling() {
    {
      boost::recursive_mutex::scoped_lock lock(pulling_mutex_);
      if (!pulling_) {
        pulling_ = true;
        BOOST_LOG_TRIVIAL(debug) << "pulling";
        io_service_.post(boost::bind(&SSLStreamBufferer::async_pull_packets,
                                     this->shared_from_this()));
      }
    }
  }

  /// User interface for receiving some data
  template <typename MutableBufferSequence, typename ReadHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
                                void(boost::system::error_code, std::size_t))
      async_read_some(const MutableBufferSequence& buffers,
                      ReadHandler&& handler) {
    boost::asio::detail::async_result_init<
        ReadHandler, void(boost::system::error_code, std::size_t)>
        init(std::forward<ReadHandler>(handler));

    auto buffer_size = boost::asio::buffer_size(buffers);

    if (buffer_size) {
      typedef io::pending_read_stream_operation<MutableBufferSequence,
                                                decltype(init.handler)> op;
      typename op::ptr p = {
          boost::asio::detail::addressof(init.handler),
          boost_asio_handler_alloc_helpers::allocate(sizeof(op), init.handler),
          0};

      p.p = new (p.v) op(buffers, init.handler);

      {
        boost::recursive_mutex::scoped_lock lock(op_queue_mutex_);
        op_queue_.push(p.p);
      }

      p.v = p.p = 0;

      io_service_.post(boost::bind(&SSLStreamBufferer::handle_data_n_ops,
                                   this->shared_from_this()));
    } else {
      io_service_.post(
          boost::bind<void>(init.handler, boost::system::error_code(), 0));
    }

    return init.result.get();
  }

 private:
  SSLStreamBufferer(p_ssl_stream_type p_socket, p_strand_type p_strand,
                    p_context_type p_ctx)
      : socket_(*p_socket),
        p_socket_(p_socket),
        strand_(*p_strand),
        p_strand_(p_strand),
        p_ctx_(p_ctx),
        io_service_(strand_.get_io_service()),
        status_(boost::system::error_code()),
        pulling_(false) {}

  /// Check if data is available for user requests
  void handle_data_n_ops() {
    if (!status_) {
      {
        boost::recursive_mutex::scoped_lock lock(pulling_mutex_);
        if ((data_queue_.size() < lower_queue_size_bound) && !pulling_) {
          start_pulling();
        }
      }

      boost::recursive_mutex::scoped_lock lock1(data_queue_mutex_);
      boost::recursive_mutex::scoped_lock lock2(op_queue_mutex_);
      if (!op_queue_.empty() && data_queue_.size()) {
        auto op = op_queue_.front();
        op_queue_.pop();

        size_t copied = op->fill_buffer(data_queue_);

        auto do_complete = [=]() {
          op->complete(boost::system::error_code(), copied);
        };
        io_service_.post(do_complete);

        io_service_.dispatch(boost::bind(&SSLStreamBufferer::handle_data_n_ops,
                                         this->shared_from_this()));
      }
    } else {
      boost::recursive_mutex::scoped_lock lock1(data_queue_mutex_);
      boost::recursive_mutex::scoped_lock lock2(op_queue_mutex_);
      if (!op_queue_.empty()) {
        auto op = op_queue_.front();
        op_queue_.pop();
        auto self = this->shared_from_this();
        auto do_complete = [op, this, self]() {
          op->complete(this->status_, 0);
        };
        io_service_.post(do_complete);

        io_service_.dispatch(boost::bind(&SSLStreamBufferer::handle_data_n_ops,
                                         this->shared_from_this()));
      }
    }
  }

  /// Receive some data
  void async_pull_packets() {
    auto self = this->shared_from_this();

    auto handler = [this, self](const boost::system::error_code& ec,
                                size_t length) {
      if (!ec) {
        {
          boost::recursive_mutex::scoped_lock lock1(this->data_queue_mutex_);
          this->data_queue_.commit(length);
        }

        if (!this->status_) {
          this->io_service_.dispatch(
              boost::bind(&SSLStreamBufferer::async_pull_packets,
                          this->shared_from_this()));
        }
      } else {
        this->status_ = ec;
        BOOST_LOG_TRIVIAL(info) << "SSL connection terminated";
      }

      this->io_service_.dispatch(boost::bind(
          &SSLStreamBufferer::handle_data_n_ops, this->shared_from_this()));
    };

    {
      boost::recursive_mutex::scoped_lock lock(data_queue_mutex_);
      boost::asio::streambuf::mutable_buffers_type bufs =
          data_queue_.prepare(receive_buffer_size);

      if (data_queue_.size() < higher_queue_size_bound) {
        auto lambda = [this, bufs, handler, self]() {
          this->socket_.async_read_some(bufs, this->strand_.wrap(handler));
        };
        strand_.dispatch(lambda);
      } else {
        pulling_ = false;
        BOOST_LOG_TRIVIAL(debug) << "not pulling";
      }
    }
  }

  /// The ssl stream to receive from
  ssl_stream_type& socket_;
  p_ssl_stream_type p_socket_;

  /// The strand to insure that read and writes are not concurrent
  strand_type& strand_;
  p_strand_type p_strand_;

  p_context_type p_ctx_;

  /// The io_service handling asynchronous operations
  boost::asio::io_service& io_service_;

  /// Errors during async_read are saved here
  boost::system::error_code status_;

  /// Handle the data received
  boost::recursive_mutex data_queue_mutex_;
  boost::asio::streambuf data_queue_;

  /// Handle pending user operations
  boost::recursive_mutex op_queue_mutex_;
  op_queue_type op_queue_;

  boost::recursive_mutex pulling_mutex_;
  bool pulling_;
};

}  // detail

/// The class wrapping an ssl stream to allow buffer optimization, non
/// concurrent io and move operations
template <typename NextLayerStreamSocket>
class basic_buffered_ssl_socket {
 private:
  typedef boost::asio::ssl::stream<NextLayerStreamSocket> ssl_stream_type;
  typedef std::shared_ptr<ssl_stream_type> p_ssl_stream_type;
  typedef detail::ExtendedSSLContext p_context_type;
  typedef std::shared_ptr<boost::asio::streambuf> p_streambuf;
  typedef detail::SSLStreamBufferer<NextLayerStreamSocket> puller_type;
  typedef std::shared_ptr<puller_type> p_puller_type;
  typedef boost::asio::io_service::strand strand_type;
  typedef std::shared_ptr<strand_type> p_strand_type;

 public:
  typedef typename ssl_stream_type::next_layer_type next_layer_type;
  typedef typename ssl_stream_type::lowest_layer_type lowest_layer_type;
  typedef typename ssl_stream_type::handshake_type handshake_type;

 public:
  basic_buffered_ssl_socket()
      : p_ctx_(nullptr),
        p_socket_(nullptr),
        socket_(),
        p_strand_(nullptr),
        p_puller_(nullptr) {}

  basic_buffered_ssl_socket(p_ssl_stream_type p_socket, p_context_type p_ctx)
      : p_ctx_(p_ctx),
        p_socket_(p_socket),
        socket_(*p_socket_),
        p_strand_(std::make_shared<strand_type>(socket_.get().lowest_layer().get_io_service())),
        p_puller_(puller_type::create(p_socket_, p_strand_, p_ctx_)) {}

  basic_buffered_ssl_socket(boost::asio::io_service& io_service,
                            p_context_type p_ctx)
      : p_ctx_(p_ctx),
        p_socket_(new ssl_stream_type(io_service, *p_ctx)),
        socket_(*p_socket_),
        p_strand_(std::make_shared<strand_type>(io_service)),
        p_puller_(puller_type::create(p_socket_, p_strand_, p_ctx_)) {}

  basic_buffered_ssl_socket(basic_buffered_ssl_socket&& other)
      : p_ctx_(std::move(other.p_ctx_)),
        p_socket_(std::move(other.p_socket_)),
        socket_(*p_socket_),
        p_strand_(std::move(other.p_strand_)),
        p_puller_(std::move(other.p_puller_)) {
    other.socket_ = *(other.p_socket_);
  }

  ~basic_buffered_ssl_socket() {}

  basic_buffered_ssl_socket(const basic_buffered_ssl_socket&) = delete;
  basic_buffered_ssl_socket& operator=(const basic_buffered_ssl_socket&) =
      delete;

  boost::asio::io_service& get_io_service() {
    return socket_.get().lowest_layer().get_io_service();
  }

  lowest_layer_type& lowest_layer() { return socket_.get().lowest_layer(); }
  next_layer_type& next_layer() { return socket_.get().next_layer(); }

  boost::system::error_code handshake(handshake_type type,
                                      boost::system::error_code& ec) {
    socket_.get().handshake(type, ec);

    if (!ec) {
      p_puller_->start_pulling();
    }

    return ec;
  }

  /// Forward the call to the ssl stream and start pulling packets on
  /// completion
  template <typename Handler>
  void async_handshake(handshake_type type, Handler handler) {
    auto do_user_handler =
        [this, handler](const boost::system::error_code& ec) mutable {
      if (!ec) {
        this->p_puller_->start_pulling();
      }
      handler(ec);
    };

    auto lambda = [this, type, do_user_handler]() {
      this->socket_.get().async_handshake(type,
                                          p_strand_->wrap(do_user_handler));
    };
    p_strand_->dispatch(lambda);
  }

  template <typename MutableBufferSequence>
  std::size_t read_some(const MutableBufferSequence& buffers,
                        boost::system::error_code& ec) {
    try {
      auto read = async_read_some(buffers, boost::asio::use_future);
      ec.assign(ssf::error::success, ssf::error::get_ssf_category());
      return read.get();
    }
    catch (const std::exception&) {
      ec.assign(ssf::error::io_error, ssf::error::get_ssf_category());
      return 0;
    }
  }

  /// Forward the call to the SSLStreamBufferer object
  template <typename MutableBufferSequence, typename ReadHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler,
                                void(boost::system::error_code, std::size_t))
      async_read_some(const MutableBufferSequence& buffers,
                      ReadHandler&& handler) {

    return p_puller_->async_read_some(buffers,
                                      std::forward<ReadHandler>(handler));
  }

  template <typename ConstBufferSequence>
  std::size_t write_some(const ConstBufferSequence& buffers,
                         boost::system::error_code& ec) {
    return socket_.get().write_some(buffers, ec);
  }

  /// Forward the call directly to the ssl stream (wrapped in an strand)
  template <typename ConstBufferSequence, typename WriteHandler>
  BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler,
                                void(boost::system::error_code, std::size_t))
      async_write_some(const ConstBufferSequence& buffers,
                       WriteHandler&& handler) {
    boost::asio::detail::async_result_init<
        WriteHandler, void(boost::system::error_code, std::size_t)>
        init(std::forward<WriteHandler>(handler));

    auto lambda = [this, buffers, init]() {
      this->socket_.get().async_write_some(buffers,
                                           this->p_strand_->wrap(init.handler));
    };
    p_strand_->dispatch(lambda);

    return init.result.get();
  }

  /// Forward the call to the lowest layer of the ssl stream
  bool is_open() { return socket_.get().lowest_layer().is_open(); }

  void close() {
    boost::system::error_code ec;
    socket_.get().lowest_layer().close(ec);
  }

  boost::system::error_code close(boost::system::error_code& ec) {
    return socket_.get().lowest_layer().close(ec);
  }

  void shutdown(boost::asio::socket_base::shutdown_type type,
                boost::system::error_code& ec) {
    socket_.get().lowest_layer().shutdown(type, ec);
  }

  boost::asio::ssl::context& context() { return *p_ctx_; }
  ssl_stream_type& socket() { return socket_; }
  strand_type& strand() { return *p_strand_; }

 private:
  /// The ssl ctx in a shared_ptr to be able to move it
  p_context_type p_ctx_;

  /// The ssl stream in a shared_ptr to be able to move it
  p_ssl_stream_type p_socket_;

  /// A reference to the ssl stream to avoid dereferencing a pointer
  std::reference_wrapper<ssl_stream_type> socket_;

  /// The strand in a shared_ptr to be able to move it
  p_strand_type p_strand_;

  /// The SSLStreamBufferer in a shared_ptr to be able to move it
  p_puller_type p_puller_;
};

//----------------------------------------------------------------------------
template <typename NextLayerStreamSocket>
class basic_ssl_socket {
 private:
  typedef boost::asio::ssl::stream<NextLayerStreamSocket> ssl_stream_type;
  typedef std::shared_ptr<ssl_stream_type> p_ssl_stream_type;
  typedef detail::ExtendedSSLContext p_context_type;
  typedef std::shared_ptr<boost::asio::streambuf> p_streambuf;
  typedef boost::asio::io_service::strand strand_type;
  typedef std::shared_ptr<strand_type> p_strand_type;

 public:
  typedef typename ssl_stream_type::next_layer_type next_layer_type;
  typedef typename ssl_stream_type::lowest_layer_type lowest_layer_type;
  typedef typename ssl_stream_type::handshake_type handshake_type;

 public:
  basic_ssl_socket()
      : p_ctx_(nullptr), p_socket_(nullptr), socket_(), p_strand_(nullptr) {}

  basic_ssl_socket(p_ssl_stream_type p_socket, p_context_type p_ctx)
      : p_ctx_(p_ctx),
        p_socket_(p_socket),
        socket_(*p_socket_),
        p_strand_(std::make_shared<strand_type>(socket_.get().lowest_layer().get_io_service())) {}

  basic_ssl_socket(boost::asio::io_service& io_service, p_context_type p_ctx)
      : p_ctx_(p_ctx),
        p_socket_(new ssl_stream_type(io_service, *p_ctx)),
        socket_(*p_socket_),
        p_strand_(std::make_shared<strand_type>(io_service)) {}

  basic_ssl_socket(basic_ssl_socket&& other)
      : p_ctx_(std::move(other.p_ctx_)),
        p_socket_(std::move(other.p_socket_)),
        socket_(*p_socket_),
        p_strand_(std::move(other.p_strand_)) {
    other.socket_ = *(other.p_socket_);
  }

  ~basic_ssl_socket() {}

  basic_ssl_socket(const basic_ssl_socket&) = delete;
  basic_ssl_socket& operator=(const basic_ssl_socket&) = delete;

  boost::asio::io_service& get_io_service() {
    return socket_.get().lowest_layer().get_io_service();
  }

  lowest_layer_type& lowest_layer() { return socket_.get().lowest_layer(); }
  next_layer_type& next_layer() { return socket_.get().next_layer(); }

  boost::system::error_code handshake(handshake_type type,
                                      boost::system::error_code ec) {
    socket_.get().handshake(type, ec);
    return ec;
  }

  /// Forward the call to the ssl stream and start pulling packets on
  /// completion
  template <typename Handler>
  void async_handshake(handshake_type type, Handler handler) {
    auto lambda = [this, type, handler]() {
      this->socket_.get().async_handshake(type, p_strand_->wrap(handler));
    };

    p_strand_->dispatch(lambda);
  }

  template <typename MutableBufferSequence>
  std::size_t read_some(const MutableBufferSequence& buffers,
                        boost::system::error_code& ec) {
    return socket_.get().read_some(buffers, ec);
  }

  /// Forward the call to the SSLStreamBufferer object
  template <typename MutableBufferSequence, typename ReadHandler>
  void async_read_some(const MutableBufferSequence& buffers,
                       ReadHandler&& handler) {
    auto lambda = [this, buffers, handler]() {
      this->socket_.get().async_read_some(buffers,
                                          this->p_strand_->wrap(handler));
    };
    p_strand_->dispatch(lambda);
  }

  template <typename ConstBufferSequence>
  std::size_t write_some(const ConstBufferSequence& buffers,
                         boost::system::error_code& ec) {
    return socket_.get().write_some(buffers, ec);
  }

  /// Forward the call directly to the ssl stream (wrapped in an strand)
  template <typename ConstBufferSequence, typename Handler>
  void async_write_some(const ConstBufferSequence& buffers, Handler&& handler) {
    auto lambda = [this, buffers, handler]() {
      this->socket_.get().async_write_some(buffers,
                                           this->p_strand_->wrap(handler));
    };
    p_strand_->dispatch(lambda);
  }

  /// Forward the call to the lowest layer of the ssl stream
  bool is_open() { return socket_.get().lowest_layer().is_open(); }

  void close() {
    boost::system::error_code ec;
    socket_.get().lowest_layer().close(ec);
  }

  boost::system::error_code close(boost::system::error_code& ec) {
    return socket_.get().lowest_layer().close(ec);
  }

  void shutdown(boost::asio::socket_base::shutdown_type type,
                boost::system::error_code& ec) {
    socket_.get().lowest_layer().shutdown(type, ec);
  }

  boost::asio::ssl::context& context() { return *p_ctx_; }
  ssl_stream_type& socket() { return socket_; }
  strand_type& strand() { return *p_strand_; }

 private:
  /// The ssl ctx in a shared_ptr to be able to move it
  p_context_type p_ctx_;

  /// The ssl stream in a shared_ptr to be able to move it
  p_ssl_stream_type p_socket_;

  /// A reference to the ssl stream to avoid dereferencing a pointer
  std::reference_wrapper<ssl_stream_type> socket_;

  /// The strand in a shared_ptr to be able to move it
  p_strand_type p_strand_;
};
//----------------------------------------------------------------------------

template <class NextLayer, template <class> class SSLStreamSocket>
class basic_ssl {
 public:
  enum {
    id = 2,
    overhead = 0,
    facilities = virtual_network::facilities::stream,
    mtu = NextLayer::mtu - overhead
  };
  enum { endpoint_stack_size = 1 + NextLayer::endpoint_stack_size };

  typedef int socket_context;
  typedef int acceptor_context;
  typedef NextLayer next_layer_protocol;
  typedef detail::ExtendedSSLContext endpoint_context_type;
  typedef virtual_network::basic_VirtualLink_endpoint<basic_ssl> endpoint;
  typedef SSLStreamSocket<typename NextLayer::socket> socket;
  typedef typename next_layer_protocol::acceptor acceptor;
  typedef typename next_layer_protocol::resolver resolver;

 public:
  template <class Container>
  static endpoint make_endpoint(
      boost::asio::io_service& io_service,
      typename Container::const_iterator parameters_it, uint32_t lower_id,
      boost::system::error_code& ec) {
    auto p_next_layer_ctx =
        detail::make_ssl_context(io_service, *parameters_it);

    if (!p_next_layer_ctx) {
      ec.assign(ssf::error::bad_address, ssf::error::get_ssf_category());
      return endpoint();
    }

    auto next_layer_endpoint = next_layer_protocol::template make_endpoint<Container>(
        io_service, ++parameters_it, lower_id, ec);

    return endpoint(p_next_layer_ctx, next_layer_endpoint);
  }
};

template <class NextLayer>
using buffered_ssl = basic_ssl<NextLayer, basic_buffered_ssl_socket>;

template <class NextLayer>
using ssl = basic_ssl<NextLayer, basic_ssl_socket>;

}  // cryptography_layer
}  // virtual_network

#endif  // SSF_CORE_VIRTUAL_NETWORK_CRYPTOGRAPHY_LAYER_SSL_H_
