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
  auto password = helpers::GetField<std::string>("password", parameters);
  auto password_lambda = [password](
      std::size_t size, boost::asio::ssl::context::password_purpose purpose)
      ->std::string {
    return password;
  };

  ctx.set_password_callback(password_lambda);

  // Set the mutual authentication
  ctx.set_verify_mode(boost::asio::ssl::verify_peer |
                      boost::asio::ssl::verify_fail_if_no_peer_cert);

  boost::system::error_code ec;

  // Set the callback to verify the cetificate chains of the peer
  ctx.set_verify_callback(&verify_certificate, ec);

  if (ec) {
    SSF_LOG(kLogError)
        << "network crypto layer: could not set verify callback";
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

  if (!SetCtxCipher(ctx, parameters)) {
    SSF_LOG(kLogError) << "network[crypto]: set context cipher suite failed";
    success = false;
  }
  if (!SetCtxCa(ctx, parameters)) {
    SSF_LOG(kLogError) << "network[crypto]: set context CA failed";
    success = false;
  }
  if (!SetCtxCrt(ctx, parameters, ec)) {
    SSF_LOG(kLogError) << "network[crypto]: set context crt failed";
    success = false;
  }
  if (!SetCtxKey(ctx, parameters, ec)) {
    SSF_LOG(kLogError) << "network[crypto]: set context key failed";
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
                  const LayerParameters& parameters) {
  auto cipher_suit =
      helpers::GetField<std::string>("set_cipher_suit", parameters);
  if (cipher_suit == "") {
    return !!SSL_CTX_set_cipher_list(ctx.native_handle(),
                                     "DHE-RSA-AES256-GCM-SHA384");
  } else {
    return !!SSL_CTX_set_cipher_list(ctx.native_handle(),
                                     cipher_suit.c_str());
  }
}

bool SetCtxCa(boost::asio::ssl::context& ctx,
              const LayerParameters& parameters) {
  if (helpers::GetField<std::string>("ca_src", parameters) == "file") {
    // Load the file containing the trusted certificate authorities
    return !!SSL_CTX_load_verify_locations(
                 ctx.native_handle(),
                 helpers::GetField<std::string>("ca_file", parameters).c_str(),
                 NULL);
  } else if (helpers::GetField<std::string>("ca_src", parameters) == "buffer") {
    auto ca_vector = get_value_in_vector(parameters, "ca_buffer");
    if (ca_vector.size()) {
      X509* x509_ca = NULL;
      ScopeCleaner cleaner([&x509_ca]() {
        X509_free(x509_ca);
        x509_ca = NULL;
      });

      auto p_ca_vector = ca_vector.data();
      auto imported = !!d2i_X509(&x509_ca, (const unsigned char**)&p_ca_vector,
                                 static_cast<long>(ca_vector.size()));

      if (!imported) {
        return false;
      }

      X509_STORE* store = X509_STORE_new();
      SSL_CTX_set_cert_store(ctx.native_handle(), store);
      auto cert_added = !!X509_STORE_add_cert(store, x509_ca);

      if (!cert_added) {
        return false;
      }

      return true;
    } else {
      return false;
    }
  }

  return false;
}

bool SetCtxCrt(boost::asio::ssl::context& ctx,
               const LayerParameters& parameters,
               boost::system::error_code& ec) {
  if (helpers::GetField<std::string>("crt_src", parameters) == "file") {
    // The certificate used by the local peer
    ctx.use_certificate_chain_file(
        helpers::GetField<std::string>("crt_file", parameters), ec);

    return !ec;
  }

  if (helpers::GetField<std::string>("crt_src", parameters) != "buffer") {
    ec.assign(ssf::error::no_crt_error, ssf::error::get_ssf_category());
    return false;
  }

  auto crt_vector = get_value_in_vector(parameters, "crt_buffer");

  if (!crt_vector.size()) {
    ec.assign(ssf::error::no_crt_error, ssf::error::get_ssf_category());
    return false;
  }

  X509* x509_cert = NULL;
  ScopeCleaner cleaner([&x509_cert]() {
    X509_free(x509_cert);
    x509_cert = NULL;
  });

  auto p_crt_vector = crt_vector.data();
  auto imported = !!d2i_X509(&x509_cert, (const unsigned char**)&p_crt_vector,
                             static_cast<long>(crt_vector.size()));

  if (!imported) {
    ec.assign(ssf::error::import_crt_error, ssf::error::get_ssf_category());
    return false;
  }

  auto used = !!SSL_CTX_use_certificate(ctx.native_handle(), x509_cert);

  if (!used) {
    ec.assign(ssf::error::set_crt_error, ssf::error::get_ssf_category());
    return false;
  }

  ec.assign(ssf::error::success, ssf::error::get_ssf_category());
  return true;
}

bool SetCtxKey(boost::asio::ssl::context& ctx,
               const LayerParameters& parameters,
               boost::system::error_code& ec) {
  if (helpers::GetField<std::string>("key_src", parameters) == "file") {
    // The private key used by the local peer
    ctx.use_private_key_file(
        helpers::GetField<std::string>("key_file", parameters),
        boost::asio::ssl::context::pem, ec);

    return !ec;
  }

  if (helpers::GetField<std::string>("key_src", parameters) != "buffer") {
    ec.assign(ssf::error::no_key_error, ssf::error::get_ssf_category());
    return false;
  }

  auto key_vector = get_value_in_vector(parameters, "key_buffer");

  if (!key_vector.size()) {
    ec.assign(ssf::error::no_key_error, ssf::error::get_ssf_category());
    return false;
  }

  EVP_PKEY* RSA_key = NULL;
  ScopeCleaner cleaner([&RSA_key]() {
    EVP_PKEY_free(RSA_key);
    RSA_key = NULL;
  });

  auto p_key_vector = key_vector.data();
  auto imported = !!d2i_PrivateKey(EVP_PKEY_RSA, &RSA_key,
                                   (const unsigned char**)&p_key_vector,
                                   static_cast<long>(key_vector.size()));

  if (!imported) {
    ec.assign(ssf::error::import_key_error, ssf::error::get_ssf_category());
    return false;
  }

  auto used = !!SSL_CTX_use_PrivateKey(ctx.native_handle(), RSA_key);

  if (!used) {
    ec.assign(ssf::error::set_key_error, ssf::error::get_ssf_category());
    return false;
  }

  ec.assign(ssf::error::success, ssf::error::get_ssf_category());
  return true;
}

bool SetCtxDhparam(boost::asio::ssl::context& ctx,
                   const LayerParameters& parameters,
                   boost::system::error_code& ec) {
  if (helpers::GetField<std::string>("dhparam_src", parameters) == "file") {
    // The Diffie-Hellman parameter file
    ctx.use_tmp_dh_file(
        helpers::GetField<std::string>("dhparam_file", parameters), ec);

    return !ec;
  } else {
    ec.assign(ssf::error::no_dh_param_error, ssf::error::get_ssf_category());
    return false;
  }
}

bool verify_certificate(bool preverified,
                        boost::asio::ssl::verify_context& ctx) {
  X509_STORE_CTX* cts = ctx.native_handle();
  X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());

  char subject_name[256];
  X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
  //std::cout << "Verifying " << subject_name << "\n";

  X509* issuer = cts->current_issuer;
  if (issuer) {
    X509_NAME_oneline(X509_get_subject_name(issuer), subject_name, 256);
    //std::cout << "Issuer " << subject_name << "\n";
  }

  // More checking ?
  return preverified;
}

std::vector<uint8_t> get_deserialized_vector(const std::string& serialized) {
  std::istringstream istrs(serialized);
  boost::archive::text_iarchive ar(istrs);
  std::vector<uint8_t> deserialized;

  ar >> BOOST_SERIALIZATION_NVP(deserialized);

  return deserialized;
}

std::vector<uint8_t> get_value_in_vector(const LayerParameters& parameters,
                                         const std::string& key) {
  if (parameters.count(key)) {
    auto serialized = parameters.find(key)->second;
    auto deserialized = get_deserialized_vector(serialized);
    return deserialized;
  } else {
    return std::vector<uint8_t>();
  }
}

}  // detail
}  // cryptography
}  // layer
}  // ssf
