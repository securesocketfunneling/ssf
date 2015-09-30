#ifndef SSF_CORE_NETWORK_VIRTUAL_LAYER_POLICIES_SSL_POLICY_H_
#define SSF_CORE_NETWORK_VIRTUAL_LAYER_POLICIES_SSL_POLICY_H_

#include <cstdint>

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <queue>

#include <boost/thread/recursive_mutex.hpp>
#include <boost/system/error_code.hpp>
#include <boost/bind.hpp>
#include <boost/log/trivial.hpp>
#include <boost/asio/connect.hpp>
#include <boost/serialization/vector.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/basic_stream_socket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>

#include "versions.h"
#include "common/config/config.h"
#include "common/error/error.h"
#include "common/boost/fiber/detail/io_ssl_read_op.hpp"
#include "common/utils/cleaner.h"

namespace ssf {

//----------------------------------------------------------------------------

/// The class in charge of receiving data from an ssl stream into a buffer
template <typename Stream = boost::asio::ip::tcp::socket>
class SSLWrapperPuller
    : public std::enable_shared_from_this<SSLWrapperPuller<Stream>> {
private:
  enum {
    lower_queue_size_bound = 1 * 1024 * 1024,
    higher_queue_size_bound = 16 * 1024 * 1024,
    receive_buffer_size = 50 * 1024
  };

private:
  typedef boost::asio::ssl::stream<Stream> ssl_type;

public:
  typedef SSLWrapperPuller<Stream> puller_type;
  typedef std::shared_ptr<puller_type> p_puller_type;
  typedef boost::asio::detail::op_queue<
      boost::asio::fiber::detail::basic_pending_ssl_operation> op_queue_type;

public:
  SSLWrapperPuller(const SSLWrapperPuller &) = delete;
  SSLWrapperPuller &operator=(const SSLWrapperPuller &) = delete;
  ~SSLWrapperPuller() {}

  static p_puller_type create(ssl_type &socket,
                              boost::asio::io_service::strand &strand) {
    return p_puller_type(new puller_type(socket, strand));
  }

  /// Start receiving data
  void start_pulling() {
    {
      boost::recursive_mutex::scoped_lock lock(pulling_mutex_);
      if (!pulling_) {
        pulling_ = true;
        BOOST_LOG_TRIVIAL(debug) << "link: SSLWrapperPuller pulling";
        io_service_.post(
            boost::bind(&SSLWrapperPuller<Stream>::async_pull_packets,
                        this->shared_from_this()));
      }
    }
  }

  /// User interface for receiving some data
  template <typename MutableBufferSequence, typename Handler>
  void async_read_some(MutableBufferSequence &buffers, Handler &handler) {
    BOOST_LOG_TRIVIAL(trace) << "link: SSLWrapperPuller async_read_some";
    auto buffer_size = boost::asio::buffer_size(buffers);

    if (buffer_size) {
      typedef boost::asio::fiber::detail::pending_ssl_read_operation<
          MutableBufferSequence, Handler> op;
      typename op::ptr p = {
          boost::asio::detail::addressof(handler),
          boost_asio_handler_alloc_helpers::allocate(sizeof(op), handler), 0};

      p.p = new (p.v) op(buffers, handler);

      {
        boost::recursive_mutex::scoped_lock lock(op_queue_mutex_);
        op_queue_.push(p.p);
      }

      p.v = p.p = 0;

      io_service_.dispatch(
          boost::bind(&SSLWrapperPuller<Stream>::handle_data_n_ops,
                      this->shared_from_this()));
    } else {
      auto lambda =
          [handler]() mutable { handler(boost::system::error_code(), 0); };
      io_service_.post(lambda);
    }
  }

private:
  SSLWrapperPuller(ssl_type &socket, boost::asio::io_service::strand &strand)
      : socket_(socket), strand_(strand), io_service_(strand.get_io_service()),
        pulling_(false) {}

  /// Check if data is available for user requests
  void handle_data_n_ops() {
    if (!status_) {
      {
        boost::recursive_mutex::scoped_lock lock(pulling_mutex_);
        if ((data_queue_.size() < lower_queue_size_bound) && !pulling_) {
          this->start_pulling();
        }
      }

      boost::recursive_mutex::scoped_lock lock1(data_queue_mutex_);
      boost::recursive_mutex::scoped_lock lock2(op_queue_mutex_);
      if (!op_queue_.empty() && data_queue_.size()) {
        auto op = op_queue_.front();
        op_queue_.pop();

        size_t copied = op->fill_buffers(data_queue_);

        auto do_complete =
            [=]() { op->complete(boost::system::error_code(), copied); };
        io_service_.post(do_complete);

        io_service_.dispatch(
            boost::bind(&SSLWrapperPuller<Stream>::handle_data_n_ops,
                        this->shared_from_this()));
      }
    } else {
      boost::recursive_mutex::scoped_lock lock1(data_queue_mutex_);
      boost::recursive_mutex::scoped_lock lock2(op_queue_mutex_);
      if (!op_queue_.empty()) {
        auto op = op_queue_.front();
        op_queue_.pop();
        auto self = this->shared_from_this();
        auto do_complete =
            [op, this, self]() { op->complete(this->status_, 0); };
        io_service_.post(do_complete);

        io_service_.dispatch(
            boost::bind(&SSLWrapperPuller<Stream>::handle_data_n_ops,
                        this->shared_from_this()));
      }
    }
  }

  /// Receive some data
  void async_pull_packets() {
    BOOST_LOG_TRIVIAL(trace) << "link: SSLWrapperPuller async_pull_packets";
    auto self = this->shared_from_this();
    auto handler = [this, self](const boost::system::error_code &ec,
                                size_t length) {
      BOOST_LOG_TRIVIAL(trace) << "link: SSLWrapperPuller async_pull_packets handler";
      if (!ec) {
        {
          boost::recursive_mutex::scoped_lock lock1(this->data_queue_mutex_);
          this->data_queue_.commit(length);
        }

        this->io_service_.dispatch(
            boost::bind(&SSLWrapperPuller<Stream>::async_pull_packets,
                        this->shared_from_this()));
      } else {
        this->status_ = ec;
        this->data_queue_.commit(length);
        BOOST_LOG_TRIVIAL(info) << "link: SSLWrapperPuller SSL connection terminated";
      }

      this->io_service_.dispatch(
          boost::bind(&SSLWrapperPuller<Stream>::handle_data_n_ops,
                      this->shared_from_this()));
    };

    {
      boost::recursive_mutex::scoped_lock lock(data_queue_mutex_);
      boost::asio::streambuf::mutable_buffers_type bufs =
          data_queue_.prepare(receive_buffer_size);

      if (data_queue_.size() < higher_queue_size_bound) {
        auto lambda = [this, bufs, handler, self]() {
          BOOST_LOG_TRIVIAL(trace)
              << "link: SSLWrapperPuller Lambda async read some buf";
          this->socket_.async_read_some(bufs, this->strand_.wrap(handler));
        };
        strand_.dispatch(lambda);
      } else {
        pulling_ = false;
        BOOST_LOG_TRIVIAL(debug) << "link: SSLWrapperPuller not pulling";
      }
    }
  }

  /// The ssl strea mto receive from
  ssl_type &socket_;

  /// The strand to insure that read and writes are not concurrent
  boost::asio::io_service::strand &strand_;

  /// The io_service handling asynchronous operations
  boost::asio::io_service &io_service_;

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
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

/// The class wrapping an ssl stream to allow buffer optimization, non
/// concurrent io and move operations
template <typename Stream = boost::asio::ip::tcp::socket> class SSLWrapper {
private:
  typedef boost::asio::ssl::stream<Stream> ssl_type;
  typedef std::shared_ptr<ssl_type> p_ssl_type;
  typedef std::shared_ptr<boost::asio::ssl::context> p_context_type;
  typedef std::shared_ptr<boost::asio::streambuf> p_streambuf;
  typedef SSLWrapperPuller<Stream> puller_type;
  typedef std::shared_ptr<SSLWrapperPuller<Stream>> p_puller_type;
  typedef boost::asio::io_service::strand strand_type;
  typedef std::shared_ptr<strand_type> p_strand_type;

public:
  typedef typename ssl_type::lowest_layer_type lowest_layer_type;
  typedef typename ssl_type::handshake_type handshake_type;

public:
  SSLWrapper(boost::asio::io_service &io_service, p_context_type p_ctx)
      : p_ctx_(p_ctx), p_socket_(new ssl_type(io_service, *p_ctx)),
        socket_(*p_socket_),
        p_strand_(std::make_shared<strand_type>(io_service)),
        p_puller_(puller_type::create(*p_socket_, *p_strand_)) {}

  SSLWrapper(SSLWrapper &&other)
      : p_ctx_(std::move(other.p_ctx_)), p_socket_(std::move(other.p_socket_)),
        socket_(*p_socket_), p_strand_(std::move(other.p_strand_)),
        p_puller_(std::move(other.p_puller_)) {
    other.socket_ = *(other.p_socket_);
  }

  SSLWrapper(const SSLWrapper &) = delete;
  SSLWrapper &operator=(const SSLWrapper &) = delete;

  boost::asio::io_service &get_io_service() {
    return socket_.get().lowest_layer().get_io_service();
  }

  lowest_layer_type &lowest_layer() { return socket_.get().lowest_layer(); }

  /// Forward the call to the ssl stream and start pulling packets on
  /// completion
  template <typename Handler>
  void async_handshake(handshake_type type, Handler handler) {
    auto do_user_handler =
        [this, handler](const boost::system::error_code &ec) {
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

  /// Forward the call to the SSLWrapperPuller object
  template <typename MutableBufferSequence, typename Handler>
  void async_read_some(const MutableBufferSequence &buffers,
                       BOOST_ASIO_MOVE_ARG(Handler) handler) {
    BOOST_LOG_TRIVIAL(trace) << "link: SSLWrapper async read some";
    p_puller_->async_read_some(buffers, handler);
  }

  /// Forward the call directly to the ssl stream (wrapped in an strand)
  template <typename ConstBufferSequence, typename Handler>
  void async_write_some(const ConstBufferSequence &buffers,
                        BOOST_ASIO_MOVE_ARG(Handler) handler) {
    auto lambda = [this, buffers, handler]() {
      BOOST_LOG_TRIVIAL(trace) << "link: SSLWrapper lambda async write some";
      this->socket_.get().async_write_some(buffers,
                                           this->p_strand_->wrap(handler));
    };
    p_strand_->dispatch(lambda);
  }

  /// Forward the call to the lowest layer of the ssl stream
  bool is_open() { return socket_.get().lowest_layer().is_open(); }

  void close(boost::system::error_code &ec) {
    socket_.get().lowest_layer().close(ec);
  }

  void shutdown(boost::asio::socket_base::shutdown_type type,
                boost::system::error_code &ec) {
    socket_.get().lowest_layer().shutdown(type, ec);
  }

  boost::asio::ssl::context &context() { return *p_ctx_; }
  boost::asio::ssl::stream<Stream> &socket() { return socket_; }
  boost::asio::io_service::strand &strand() { return *p_strand_; }

private:
  /// The ssl ctx in a shared_ptr to be able to move it
  p_context_type p_ctx_;

  /// The ssl stream in a shared_ptr to be able to move it
  p_ssl_type p_socket_;

  /// A reference to the ssl stream to avoid dereferencing a pointer
  std::reference_wrapper<ssl_type> socket_;

  /// The strand in a shared_ptr to be able to move it
  p_strand_type p_strand_;

  /// The SSLWrapperPuller in a shared_ptr to be able to move it
  p_puller_type p_puller_;
};
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

/// The class which handles SSL links
template <typename StreamSocket = boost::asio::ip::tcp::socket>
class BaseSSLPolicy {
public:
  /// The socket object type used
  typedef SSLWrapper<StreamSocket> socket_type;

  /// The pointer type returned to the higher layer
  typedef std::shared_ptr<socket_type> p_socket_type;

  /// The acceptor object type
  typedef boost::asio::ip::tcp::acceptor acceptor_type;
  typedef std::shared_ptr<acceptor_type> p_acceptor_type;

  /// Type of the higher layer callback for establishing a new link
  typedef std::function<void(p_socket_type p_socket,
                             boost::system::error_code &)>
      connect_callback_type;

  /// Type of the higher layer callback for accepting new links
  typedef std::function<void(p_socket_type p_socket)> accept_callback_type;

  /// Type of the generalized parameters
  typedef std::map<std::string, std::string> Parameters;

public:
  virtual ~BaseSSLPolicy() {}

  BaseSSLPolicy(boost::asio::io_service &io_service,
                const ssf::Config &ssf_config)
      : io_service_(io_service), config_(ssf_config) {}

private:
  bool init_ssl_context(boost::asio::ssl::context &ctx, bool is_server) {
    // Set the callback to decipher the private key
    if (is_server || (config_.tls.key_password != "")) {
      ctx.set_password_callback(
        boost::bind(&BaseSSLPolicy::get_password, this, _1, _2));
    }

    // Set the mutual authentication
    ctx.set_verify_mode(boost::asio::ssl::verify_peer |
                        boost::asio::ssl::verify_fail_if_no_peer_cert);

    // Set the callback to verify the cetificate chains of the peer
    ctx.set_verify_callback(boost::bind(
        &BaseSSLPolicy<StreamSocket>::verify_certificate, this, _1, _2));

    // Set various security options
    ctx.set_options(boost::asio::ssl::context::default_workarounds |
                    boost::asio::ssl::context::no_sslv2 |
                    boost::asio::ssl::context::no_sslv3 |
                    boost::asio::ssl::context::no_tlsv1 |
                    boost::asio::ssl::context::single_dh_use);

    SSL_CTX_set_options(ctx.native_handle(),
                        SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TICKET);

    // [not used] Set compression methods
    SSL_COMP_add_compression_method(0, COMP_rle());
    SSL_COMP_add_compression_method(1, COMP_zlib());

    // Load the file containing the trusted certificate authorities
    auto loaded = SSL_CTX_load_verify_locations(
        ctx.native_handle(), config_.tls.ca_cert_path.c_str(), NULL);

    if (!loaded) {
      return false;
    }

    boost::system::error_code ec;

    // The certificate used by the local peer
    ctx.use_certificate_chain_file(config_.tls.cert_path, ec);

    if (ec) {
      return false;
    }

    // The private key used by the local peer
    ctx.use_private_key_file(config_.tls.key_path.c_str(),
                             boost::asio::ssl::context::pem, ec);

    if (ec) {
      return false;
    }

    // The Diffie-Hellman parameter file
    ctx.use_tmp_dh_file(config_.tls.dh_path, ec);

    return !ec;
  }

public:
  /// Connect to a remote server
  void establish_link(const Parameters &parameters,
                      const connect_callback_type &connect_callback) {
    auto addr = get_remote_addr(parameters);
    auto port = get_remote_port(parameters);
    auto auth_cert = get_auth_cert_in_vector(parameters);
    auto cert = get_cert_in_vector(parameters);
    auto key = get_key_in_vector(parameters);

    BOOST_LOG_TRIVIAL(info) << "link: connecting to " << addr << ":" << port;

    auto p_ctx = std::make_shared<boost::asio::ssl::context>(
        boost::asio::ssl::context::tlsv12);

    // Initialize the ssl context with enhanced security parameters
    auto ctx_set = this->init_ssl_context(*p_ctx, false);

    if (!ctx_set) {
      boost::system::error_code ec(ssf::error::invalid_argument,
                                   ssf::error::get_ssf_category());
      this->to_next_layer_handler(p_socket_type(nullptr), connect_callback, ec);
      return;
    }

    // If an other certificate and key are provided, use them
    if (cert.size() && key.size()) {
      boost::system::error_code ec;

      // If a new CA is provided, use it
      if (auth_cert.size()) {
        X509 *x509_ca = NULL;
        ScopeCleaner cleaner([&x509_ca]() {
          X509_free(x509_ca);
          x509_ca = NULL;
        });

        auto p_auth_cert = auth_cert.data();
        d2i_X509(&x509_ca, (const unsigned char **)&p_auth_cert,
                 (uint32_t)auth_cert.size());
        X509_STORE *store = X509_STORE_new();
        SSL_CTX_set_cert_store(p_ctx->native_handle(), store);
        X509_STORE_add_cert(store, x509_ca);
      }

      {
        X509 *x509_cert = NULL;
        ScopeCleaner cleaner([&x509_cert]() {
          X509_free(x509_cert);
          x509_cert = NULL;
        });

        auto p_cert = cert.data();
        d2i_X509(&x509_cert, (const unsigned char **)&p_cert, (uint32_t)cert.size());
        SSL_CTX_use_certificate(p_ctx->native_handle(), x509_cert);
      }

      {
        EVP_PKEY *RSA_key = NULL;
        ScopeCleaner cleaner([&RSA_key]() {
          EVP_PKEY_free(RSA_key);
          RSA_key = NULL;
        });

        auto p_key = key.data();
        d2i_PrivateKey(EVP_PKEY_RSA, &RSA_key, (const unsigned char **)&p_key,
                       (uint32_t)key.size());
        SSL_CTX_use_PrivateKey(p_ctx->native_handle(), RSA_key);
      }
    }

    // If the address and port are valid
    if (addr != "" && port != "") {
      // Resolve the address
      boost::asio::ip::tcp::resolver resolver(io_service_);
      boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(),
                                                  addr, port);
      
      boost::system::error_code resolve_ec;
      auto iterator = resolver.resolve(query, resolve_ec);

      if (resolve_ec) {
        BOOST_LOG_TRIVIAL(error) << "link: could not resolve " << addr << ":"
                                 << port;
        boost::system::error_code ec(ssf::error::invalid_argument,
                                     ssf::error::get_ssf_category());
        this->to_next_layer_handler(p_socket_type(nullptr), connect_callback,
                                    ec);
        return;
      }

      auto p_socket = std::make_shared<socket_type>(io_service_, p_ctx);

      boost::asio::async_connect(p_socket->lowest_layer(), iterator,
                                 boost::bind(&BaseSSLPolicy::connected_handler,
                                             this, p_socket, connect_callback,
                                             _1));
    } else {
      boost::system::error_code ec(ssf::error::invalid_argument,
                                   ssf::error::get_ssf_category());
      this->to_next_layer_handler(p_socket_type(nullptr), connect_callback, ec);
      return;
    }
  }

  /// Accept a new connection
  void accept_links(p_acceptor_type p_acceptor,
                    const accept_callback_type &accept_callback) {

    if (!p_acceptor->is_open()) {
      return;
    }

    auto p_ctx = std::make_shared<boost::asio::ssl::context>(
        boost::asio::ssl::context::tlsv12);

    // Initialize the ssl context with enhanced security parameters
    auto ctx_set = this->init_ssl_context(*p_ctx, true);

    if (!ctx_set) {
      this->to_next_layer_handler(nullptr, accept_callback);
      return;
    }

    // Force a strong cipher suite by default
    SSL_CTX_set_cipher_list(p_ctx->native_handle(),
                            config_.tls.cipher_alg.c_str());

    auto p_socket = std::make_shared<socket_type>(io_service_, p_ctx);

    BOOST_LOG_TRIVIAL(trace) << "link: accepting";
    p_acceptor->async_accept(p_socket->lowest_layer(),
                             boost::bind(&BaseSSLPolicy::accepted_handler, this,
                                         p_acceptor, p_socket, accept_callback,
                                         _1));
  }

  void close_link(socket_type &socket) {
    boost::system::error_code ec;
    socket.socket().shutdown(ec);
    socket.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket.lowest_layer().close(ec);
    BOOST_LOG_TRIVIAL(info) << "link: SSL connection closed";
  }

private:
  void connected_handler(p_socket_type p_socket,
                         const connect_callback_type &connect_callback,
                         const boost::system::error_code &ec) {
    // Send version
    if (!ec) {
      std::shared_ptr<uint32_t> p_version = std::make_shared<uint32_t>(GetVersion());
      /*boost::asio::async_write(p_socket->socket().next_layer(),
                               boost::asio::buffer(p_version.get(), sizeof(*p_version)),
                               boost::bind(&BaseSSLPolicy::handshake_handler, this,
                               p_socket, p_version, connect_callback, _1, _2));*/
      handshake_handler(p_socket, p_version, connect_callback, ec, 0);
    } else {
      BOOST_LOG_TRIVIAL(error) << "link: connection failed "
        << ec.message();
      this->close_link(*p_socket);
      this->to_next_layer_handler(nullptr, connect_callback, ec);
    }
  }

  void handshake_handler(p_socket_type p_socket,
                         std::shared_ptr<uint32_t> p_version,
                         const connect_callback_type &connect_callback,
                         const boost::system::error_code &ec, size_t length) {
    if (!ec) {
      BOOST_LOG_TRIVIAL(trace) << "link: connected";
      p_socket->async_handshake(
          boost::asio::ssl::stream_base::client,
          boost::bind(&BaseSSLPolicy::handshaked_connect_handler, this,
                      p_socket, connect_callback, _1));
    } else {
      BOOST_LOG_TRIVIAL(error) << "link: exchange version failed "
                               << ec.message();
      this->close_link(*p_socket);
      this->to_next_layer_handler(nullptr, connect_callback, ec);
    }
  }

  void handshaked_connect_handler(p_socket_type p_socket,
                                  const connect_callback_type &connect_callback,
                                  const boost::system::error_code &ec) {
    if (!ec) {
      BOOST_LOG_TRIVIAL(trace) << "link: authenticated";
      this->to_next_layer_handler(p_socket, connect_callback, ec);
    } else {
      BOOST_LOG_TRIVIAL(error) << "link: no handshake " << ec.message();
      this->close_link(*p_socket);
      this->to_next_layer_handler(nullptr, connect_callback, ec);
    }
  }

  void to_next_layer_handler(p_socket_type p_socket,
                             const connect_callback_type &connect_callback,
                             const boost::system::error_code &ec) {
    io_service_.post(boost::bind(connect_callback, p_socket, ec));
  }

  /// Read ssl version
  void accepted_handler(p_acceptor_type p_acceptor, p_socket_type p_socket,
                        const accept_callback_type &accept_callback,
                        const boost::system::error_code &ec) {
    if (!ec) {
      BOOST_LOG_TRIVIAL(trace) << "link: accepted";
      /*std::shared_ptr<uint32_t> p_version = std::make_shared<uint32_t>();
      boost::asio::async_read(p_socket->socket().next_layer(),
                              boost::asio::buffer(p_version.get(), sizeof(*p_version)),
                              boost::bind(&BaseSSLPolicy::accepted_handshake_handler, this,
                                          p_socket, p_version, accept_callback, _1, _2));*/
      std::shared_ptr<uint32_t> p_version =
          std::make_shared<uint32_t>(GetVersion());
      accepted_handshake_handler(p_socket, p_version, accept_callback, ec, 0);
      this->accept_links(p_acceptor, accept_callback);
    } else {
      this->close_link(*p_socket);
      this->to_next_layer_handler(nullptr, accept_callback);
    }
  }

  /// Operate the ssl handshake if version supported and no error
  void accepted_handshake_handler(p_socket_type p_socket,
                                  std::shared_ptr<uint32_t> p_version,
                                  const accept_callback_type &accept_callback,
                                  const boost::system::error_code &ec,
                                  size_t bytes_transferred) {
    auto version_supported = IsVersionSupported(*p_version);
    if (!ec && version_supported) {
      BOOST_LOG_TRIVIAL(trace) << "link: version supported";
      p_socket->async_handshake(
          boost::asio::ssl::stream_base::server,
          boost::bind(&BaseSSLPolicy::handshaked_accept_handler, this, p_socket,
                      accept_callback, _1));
    } else {
      if (!version_supported) {
        BOOST_LOG_TRIVIAL(error) << "link: version NOT supported "
                                 << *p_version;
      }
      if (ec) {
        BOOST_LOG_TRIVIAL(error) << "link: error on read version "
                                 << "ec : " << ec.message();
      }
      this->close_link(*p_socket);
      this->to_next_layer_handler(nullptr, accept_callback);
    }
  }

  void handshaked_accept_handler(p_socket_type p_socket,
                                 const accept_callback_type &accept_callback,
                                 const boost::system::error_code &ec) {
    if (!ec) {
      BOOST_LOG_TRIVIAL(trace) << "link: authenticated";
      if (p_socket) {
        BOOST_LOG_TRIVIAL(trace)
            << "link: cipher suite "
            << SSL_get_cipher(p_socket->socket().native_handle());
      }
      this->to_next_layer_handler(p_socket, accept_callback);
    } else {
      BOOST_LOG_TRIVIAL(error) << "link: NOT Authenticated " << ec.message();
      this->close_link(*p_socket);
      this->to_next_layer_handler(nullptr, accept_callback);
    }
  }

  void to_next_layer_handler(p_socket_type p_socket,
                             const accept_callback_type &accept_callback) {
    io_service_.post(boost::bind(accept_callback, p_socket));
  }

  uint32_t GetVersion() {
    uint32_t version = ssf::versions::Versions::major;
    version = version << 8;

    version |= ssf::versions::Versions::minor;
    version = version << 8;

    version |= ssf::versions::Versions::security;
    version = version << 8;

    version |= uint8_t(boost::archive::BOOST_ARCHIVE_VERSION());

    return version;
  }

  bool IsVersionSupported(uint32_t input_version) {
    boost::archive::library_version_type serialization(input_version &
                                                       0x000000FF);
    input_version = input_version >> 8;

    uint8_t security = (input_version & 0x000000FF);
    input_version = input_version >> 8;

    uint8_t minor = (input_version & 0x000000FF);
    input_version = input_version >> 8;

    uint8_t major = (input_version & 0x000000FF);

    return (major == ssf::versions::Versions::major) &&
      (minor == ssf::versions::Versions::minor) &&
      (security == ssf::versions::Versions::security) &&
      (serialization == boost::archive::BOOST_ARCHIVE_VERSION());
  }

  std::string get_password(std::size_t,
                           boost::asio::ssl::context::password_purpose) const {
    return config_.tls.key_password;
  }

  bool verify_certificate(bool preverified,
                          boost::asio::ssl::verify_context &ctx) {

    X509_STORE_CTX *cts = ctx.native_handle();
    X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());

    char subject_name[256];
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
    BOOST_LOG_TRIVIAL(info) << "link: verifying " << subject_name << "\n";

    X509 *issuer = cts->current_issuer;
    if (issuer) {
      X509_NAME_oneline(X509_get_subject_name(issuer), subject_name, 256);
      BOOST_LOG_TRIVIAL(info) << "link: issuer " << subject_name << "\n";
    }

    // More checking ?
    return preverified;
  }

  std::string get_remote_addr(const Parameters &parameters) {
    if (parameters.count("remote_addr")) {
      return parameters.find("remote_addr")->second;
    } else {
      return "";
    }
  }

  std::string get_remote_port(const Parameters &parameters) {
    if (parameters.count("remote_port")) {
      return parameters.find("remote_port")->second;
    } else {
      return "";
    }
  }

  std::vector<uint8_t> get_deserialized_vector(const std::string &serialized) {
    std::istringstream istrs(serialized);
    boost::archive::text_iarchive ar(istrs);
    std::vector<uint8_t> deserialized;

    try {
      ar >> BOOST_SERIALIZATION_NVP(deserialized);
      return deserialized;
    } catch (const std::exception&) {
      return std::vector<uint8_t>(0);
    }
  }

  std::vector<uint8_t> get_auth_cert_in_vector(const Parameters &parameters) {
    if (parameters.count("auth_cert_in_vector")) {
      auto serialized = parameters.find("auth_cert_in_vector")->second;
      auto deserialized = get_deserialized_vector(serialized);
      return deserialized;
    } else {
      return std::vector<uint8_t>();
    }
  }

  std::vector<uint8_t> get_cert_in_vector(const Parameters &parameters) {
    if (parameters.count("cert_in_vector")) {
      auto serialized = parameters.find("cert_in_vector")->second;
      auto deserialized = get_deserialized_vector(serialized);
      return deserialized;
    } else {
      return std::vector<uint8_t>();
    }
  }

  std::vector<uint8_t> get_key_in_vector(const Parameters &parameters) {
    if (parameters.count("key_in_vector")) {
      auto serialized = parameters.find("key_in_vector")->second;
      auto deserialized = get_deserialized_vector(serialized);
      return deserialized;
    } else {
      return std::vector<uint8_t>();
    }
  }

private:
  boost::asio::io_service &io_service_;
  ssf::Config config_;
};

typedef BaseSSLPolicy<> SSLPolicy;
//----------------------------------------------------------------------------

} // ssf

#endif // SSF_CORE_NETWORK_VIRTUAL_LAYER_POLICIES_SSL_POLICY_H_
