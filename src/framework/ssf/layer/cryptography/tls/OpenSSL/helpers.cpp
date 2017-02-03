#include "ssf/layer/cryptography/tls/OpenSSL/helpers.h"

#include <boost/archive/text_iarchive.hpp>

#include <boost/serialization/vector.hpp>

#include "ssf/error/error.h"
#include "ssf/log/log.h"
#include "ssf/utils/cleaner.h"
#include "ssf/utils/map_helpers.h"

namespace ssf {
namespace layer {
namespace cryptography {
namespace detail {

ExtendedTLSContext::ExtendedTLSContext(
    std::shared_ptr<boost::asio::ssl::context> p_ctx)
    : p_ctx_(std::move(p_ctx)) {}

ExtendedTLSContext::~ExtendedTLSContext() {}

boost::asio::ssl::context& ExtendedTLSContext::operator*() { return *p_ctx_; }
std::shared_ptr<boost::asio::ssl::context> ExtendedTLSContext::operator->() {
  return p_ctx_;
}

bool ExtendedTLSContext::operator==(const ExtendedTLSContext&) const {
  return true;
}

bool ExtendedTLSContext::operator!=(const ExtendedTLSContext&) const {
  return false;
}

bool ExtendedTLSContext::operator<(const ExtendedTLSContext& other) const {
  return p_ctx_ < other.p_ctx_;
}

bool ExtendedTLSContext::operator!() const { return !p_ctx_; }

ExtendedTLSContext make_tls_context(boost::asio::io_service& io_service,
                                    const LayerParameters& parameters) {
  auto p_ctx = std::make_shared<boost::asio::ssl::context>(
      boost::asio::ssl::context::tlsv12);

  auto& ctx = *p_ctx;

  // Set the callback to decipher the private key
  auto key_password =
      helpers::GetField<std::string>("key_password", parameters);
  auto password_lambda =
      [key_password](
          std::size_t size,
          boost::asio::ssl::context::password_purpose purpose) -> std::string {
        return key_password;
      };

  ctx.set_password_callback(password_lambda);

  // Set the mutual authentication
  ctx.set_verify_mode(boost::asio::ssl::verify_peer |
                      boost::asio::ssl::verify_fail_if_no_peer_cert);

  boost::system::error_code ec;

  // Set the callback to verify the cetificate chains of the peer
  ctx.set_verify_callback(&VerifyCertificate, ec);

  if (ec) {
    SSF_LOG(kLogError) << "network[crypto]: could not set verify callback";
    return ExtendedTLSContext(nullptr);
  }

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

  bool success = true;

  if (!SetCtxCipher(ctx, parameters, ec)) {
    SSF_LOG(kLogError) << "network[crypto]: set context cipher suite failed: "
                       << ec.message();
    success = false;
  }
  if (!SetCtxCa(ctx, parameters, ec)) {
    SSF_LOG(kLogError) << "network[crypto]: set context CA failed: "
                       << ec.message();
    success = false;
  }
  if (!SetCtxCrt(ctx, parameters, ec)) {
    SSF_LOG(kLogError) << "network[crypto]: set context crt failed: "
                       << ec.message();
    success = false;
  }
  if (!SetCtxKey(ctx, parameters, ec)) {
    SSF_LOG(kLogError) << "network[crypto]: set context key failed: "
                       << ec.message();
    success = false;
  }

  success |= SetCtxDhparam(ctx, parameters, ec);

  if (!success) {
    SSF_LOG(kLogError) << "network[crypto]: context init failed";
    return ExtendedTLSContext(nullptr);
  }

  return ExtendedTLSContext(p_ctx);
}

bool SetCtxCipher(boost::asio::ssl::context& ctx,
                  const LayerParameters& parameters,
                  boost::system::error_code& ec) {
  if (parameters.count("cipher_suit") > 0) {
    return !!SSL_CTX_set_cipher_list(ctx.native_handle(),
                                     helpers::GetField<std::string>(
                                         "cipher_suit", parameters).c_str());
  } else {
    return !!SSL_CTX_set_cipher_list(ctx.native_handle(),
                                     "DHE-RSA-AES256-GCM-SHA384");
  }
}

bool SetCtxCa(boost::asio::ssl::context& ctx, const LayerParameters& parameters,
              boost::system::error_code& ec) {
  if (parameters.count("ca_file") > 0) {
    ctx.load_verify_file(helpers::GetField<std::string>("ca_file", parameters),
                         ec);
    return !ec;
  } else if (parameters.count("ca_buffer") > 0) {
    auto ca_vector = helpers::GetField<std::string>("ca_buffer", parameters);
    if (ca_vector.empty()) {
      return false;
    }
    ctx.add_certificate_authority(boost::asio::buffer(ca_vector), ec);
    return !ec;
  }
  return false;
}

bool SetCtxCrt(boost::asio::ssl::context& ctx,
               const LayerParameters& parameters,
               boost::system::error_code& ec) {
  if (parameters.count("crt_file") > 0) {
    ctx.use_certificate_chain_file(
        helpers::GetField<std::string>("crt_file", parameters), ec);
    return !ec;
  } else if (parameters.count("crt_buffer") > 0) {
    auto crt_vector = helpers::GetField<std::string>("crt_buffer", parameters);
    if (crt_vector.empty()) {
      ec.assign(ssf::error::no_crt_error, ssf::error::get_ssf_category());
      return false;
    }
    ctx.use_certificate_chain(boost::asio::buffer(crt_vector), ec);
    return !ec;
  } else {
    ec.assign(ssf::error::no_crt_error, ssf::error::get_ssf_category());
    return false;
  }
}

bool SetCtxKey(boost::asio::ssl::context& ctx,
               const LayerParameters& parameters,
               boost::system::error_code& ec) {
  if (parameters.count("key_file") > 0) {
    // The private key used by the local peer
    ctx.use_private_key_file(
        helpers::GetField<std::string>("key_file", parameters),
        boost::asio::ssl::context::pem, ec);
    return !ec;
  } else if (parameters.count("key_buffer") > 0) {
    std::string key_buffer =
        helpers::GetField<std::string>("key_buffer", parameters);

    if (key_buffer.empty()) {
      ec.assign(ssf::error::no_key_error, ssf::error::get_ssf_category());
      return false;
    }
    ctx.use_private_key(boost::asio::buffer(key_buffer),
                        boost::asio::ssl::context::pem, ec);
    return !ec;
  } else {
    ec.assign(ssf::error::no_key_error, ssf::error::get_ssf_category());
    return false;
  }
}

bool SetCtxDhparam(boost::asio::ssl::context& ctx,
                   const LayerParameters& parameters,
                   boost::system::error_code& ec) {
  if (parameters.count("dhparam_file") > 0) {
    ctx.use_tmp_dh_file(
        helpers::GetField<std::string>("dhparam_file", parameters), ec);

    return !ec;
  } else if (parameters.count("dhparam_buffer") > 0) {
    std::string dh_buffer =
        helpers::GetField<std::string>("dhparam_buffer", parameters);
    if (dh_buffer.empty()) {
      ec.assign(ssf::error::no_key_error, ssf::error::get_ssf_category());
      return false;
    }
    ctx.use_tmp_dh(boost::asio::buffer(dh_buffer), ec);
    return !ec;
  } else {
    ec.assign(ssf::error::no_dh_param_error, ssf::error::get_ssf_category());
    return false;
  }
}

bool VerifyCertificate(bool preverified,
                       boost::asio::ssl::verify_context& ctx) {
  X509_STORE_CTX* peer_cert = ctx.native_handle();

  int32_t depth = X509_STORE_CTX_get_error_depth(peer_cert);
  if (depth != 0) {
    return preverified;
  }

  X509* cert = X509_STORE_CTX_get_current_cert(peer_cert);
  char subject_buf[256] = {0};
  char issuer_buf[256] = {0};
  X509* issuer = peer_cert->current_issuer;
  if (issuer) {
    X509_NAME_oneline(X509_get_issuer_name(issuer), issuer_buf, 256);
  }

  X509_NAME_oneline(X509_get_subject_name(cert), subject_buf, 256);
  SSF_LOG(kLogDebug) << "network[crypto]: verify cert (subject=" << subject_buf
                    << ", issuer=" << issuer_buf << ")";

  // More checking ?
  return preverified;
}

}  // detail
}  // cryptography
}  // layer
}  // ssf
