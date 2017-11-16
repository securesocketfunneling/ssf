#ifndef SSF_COMMON_CRYPTO_SHA1_H_
#define SSF_COMMON_CRYPTO_SHA1_H_

#include <cstdint>

#include <array>
#include <vector>

#include <openssl/sha.h>

#include "common/filesystem/path.h"

namespace ssf {
namespace crypto {

class Sha1 {
 public:
  using Digest = std::array<uint8_t, SHA_DIGEST_LENGTH>;
  using Buffer = std::vector<uint8_t>;

 public:
  Sha1();

  template <class Buffer>
  void Update(const Buffer& buffer, std::size_t length) {
    SHA_Update(&context_, buffer.data(), length);
  }

  void Finalize(Digest* digest);

 private:
  SHA_CTX context_;
};

}  // crypto
}  // ssf

#endif  // SSF_COMMON_CRYPTO_SHA1_H_