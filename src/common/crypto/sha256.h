#ifndef SSF_COMMON_CRYPTO_SHA256_H_
#define SSF_COMMON_CRYPTO_SHA256_H_

#include <cstdint>

#include <array>
#include <vector>

#include <openssl/sha.h>

#include "common/filesystem/path.h"

namespace ssf {
namespace crypto {

class Sha256 {
 public:
  using Digest = std::array<uint8_t, SHA256_DIGEST_LENGTH>;
  using Buffer = std::vector<uint8_t>;

 public:
  Sha256();

  template <class Buffer>
  void Update(const Buffer& buffer, std::size_t length) {
    SHA256_Update(&context_, buffer.data(), length);
  }

  void Finalize(Digest* digest);

 private:
  SHA256_CTX context_;
};

}  // crypto
}  // ssf

#endif  // SSF_COMMON_CRYPTO_SHA1_H_