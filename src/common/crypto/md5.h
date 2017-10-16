#ifndef SSF_COMMON_CRYPTO_MD5_H_
#define SSF_COMMON_CRYPTO_MD5_H_

#include <cstdint>

#include <array>
#include <vector>

#include <openssl/md5.h>

#include "common/filesystem/path.h"

namespace ssf {
namespace crypto {

class Md5 {
 public:
  using Digest = std::array<uint8_t, MD5_DIGEST_LENGTH>;
  using Buffer = std::vector<uint8_t>;

 public:
  Md5();

  template <class Buffer>
  void Update(const Buffer& buffer, std::size_t length) {
    MD5_Update(&context_, buffer.data(), length);
  }

  void Finalize(Digest* digest);

 private:
  MD5_CTX context_;
};

}  // crypto
}  // ssf

#endif  // SSF_COMMON_CRYPTO_MD5_H_