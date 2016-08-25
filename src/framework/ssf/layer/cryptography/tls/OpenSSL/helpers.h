#ifndef SSF_LAYER_CRYPTOGRAPHY_TLS_OPENSSL_HELPERS_H_
#define SSF_LAYER_CRYPTOGRAPHY_TLS_OPENSSL_HELPERS_H_

#include <cstdint>

#include <map>
#include <memory>
#include <string>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ssl.hpp>

#include "ssf/layer/parameters.h"

namespace ssf {
namespace layer {
namespace cryptography {
namespace detail {

struct ExtendedTLSContext {
  ExtendedTLSContext() : p_ctx_(nullptr) {}

  explicit ExtendedTLSContext(std::shared_ptr<boost::asio::ssl::context> p_ctx);

  boost::asio::ssl::context& operator*();
  std::shared_ptr<boost::asio::ssl::context> operator->();

  bool operator==(const ExtendedTLSContext&) const;
  bool operator!=(const ExtendedTLSContext&) const;
  bool operator<(const ExtendedTLSContext& other) const;
  bool operator!() const;

  std::shared_ptr<boost::asio::ssl::context> p_ctx_;
};

ExtendedTLSContext make_tls_context(boost::asio::io_service& io_service,
                                    const LayerParameters& parameters);
bool SetCtxCipher(boost::asio::ssl::context& ctx,
                  const LayerParameters& parameters);
bool SetCtxCa(boost::asio::ssl::context& ctx,
              const LayerParameters& parameters);
bool SetCtxCrt(boost::asio::ssl::context& ctx,
               const LayerParameters& parameters,
               boost::system::error_code& ec);
bool SetCtxKey(boost::asio::ssl::context& ctx,
               const LayerParameters& parameters,
               boost::system::error_code& ec);
bool SetCtxDhparam(boost::asio::ssl::context& ctx,
                   const LayerParameters& parameters,
                   boost::system::error_code& ec);
bool verify_certificate(bool preverified,
                        boost::asio::ssl::verify_context& ctx);
std::vector<uint8_t> get_deserialized_vector(const std::string& serialized);
std::vector<uint8_t> get_value_in_vector(const LayerParameters& parameters,
                                         const std::string& key);

}  // detail
}  // cryptography
}  // layer
}  // ssf

#endif  // SSF_LAYER_CRYPTOGRAPHY_TLS_OPENSSL_HELPERS_H_
