#include "common/crypto/sha256.h"

namespace ssf {
namespace crypto {

Sha256::Sha256() : context_() { SHA256_Init(&context_); }

void Sha256::Finalize(Digest* digest) {
  SHA256_Final(digest->data(), &context_);
}

}  // crypto
}  // ssf