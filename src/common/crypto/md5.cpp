#include "common/crypto/md5.h"

namespace ssf {
namespace crypto {

Md5::Md5() : context_() { MD5_Init(&context_); }

void Md5::Finalize(Digest* digest) { MD5_Final(digest->data(), &context_); }

}  // crypto
}  // ssf