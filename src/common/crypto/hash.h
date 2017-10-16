#ifndef SSF_COMMON_CRYPTO_HASH_H_
#define SSF_COMMON_CRYPTO_HASH_H_

#include <iostream>

#include <boost/system/error_code.hpp>

#include "common/error/error.h"
#include "common/filesystem/filesystem.h"
#include "common/filesystem/path.h"

namespace ssf {
namespace crypto {

template <class Hash>
typename Hash::Digest HashFile(const ssf::Path& path,
                               boost::system::error_code& ec) {
  ssf::Filesystem fs;
  auto filesize = fs.GetFilesize(path, ec);
  if (ec) {
    SSF_LOG(kLogError) << "hash: could not get filesize of "
                       << path.GetString();
    return {};
  }
  return HashFile<Hash>(path, filesize, ec);
}

template <class Hash>
typename Hash::Digest HashFile(const ssf::Path& path, uint64_t stop_offset,
                               boost::system::error_code& ec) {
  typename Hash::Digest digest;
  std::array<char, 4096> buffer;

  std::ifstream file(path.GetString(),
                     std::ifstream::binary | std::ifstream::in);
  if (!file.is_open()) {
    ec.assign(boost::system::errc::bad_file_descriptor,
              boost::system::get_system_category());
    return {};
  }

  Hash hash;

  uint64_t remaining = stop_offset;
  uint64_t read_len;
  do {
    read_len = (remaining < buffer.size()) ? remaining : buffer.size();
    file.read(buffer.data(), read_len);
    remaining -= file.gcount();
    hash.Update(buffer, static_cast<std::size_t>(file.gcount()));
  } while (!file.eof() && remaining > 0);

  hash.Finalize(&digest);

  return digest;
}

}  // crypto
}  // ssf

#endif  // SSF_COMMON_CRYPTO_HASH_H_