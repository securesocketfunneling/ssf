#ifndef SSF_LAYER_PROXY_DIGEST_AUTH_STRATEGY_H_
#define SSF_LAYER_PROXY_DIGEST_AUTH_STRATEGY_H_

#include <cstdint>

#include <array>
#include <map>
#include <string>

#include "ssf/layer/proxy/auth_strategy.h"

namespace ssf {
namespace layer {
namespace proxy {

class DigestAuthStrategy : public AuthStrategy {
 private:
  using Md5Digest = std::array<unsigned char, 16>;

  enum Qop { kNone, kAuth, kAuthInt };

 public:
  DigestAuthStrategy(const Proxy& proxy_ctx);

  virtual ~DigestAuthStrategy(){};

  std::string AuthName() const override;

  inline void set_cnonce(const std::string& cnonce) { cnonce_ = cnonce; }

  bool Support(const HttpResponse& response) const override;

  void ProcessResponse(const HttpResponse& response) override;

  void PopulateRequest(HttpRequest* p_request) override;

 private:
  void ParseDigestChallenge(const HttpResponse& response);
  std::string GenerateResponseDigest(const HttpRequest& request);
  std::string GenerateA1Hash();
  std::string GenerateA2Hash(const HttpRequest& request);
  static std::string GenerateRandomString(std::size_t strlen);
  static std::string BufferToHex(unsigned char* buffer, std::size_t buffer_len);

 private:
  std::map<std::string, std::string> challenge_;
  bool request_populated_;
  Qop qop_;
  std::string cnonce_;
  uint32_t nonce_count_;
};

}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_DIGEST_AUTH_STRATEGY_H_