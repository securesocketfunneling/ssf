#include "common/crypto/sha1.h"

namespace ssf {
namespace crypto {

Sha1::Sha1() : context_() { SHA_Init(&context_); }

void Sha1::Finalize(Digest* digest) { SHA_Final(digest->data(), &context_); }

}  // crypto
}  // ssf