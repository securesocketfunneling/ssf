#include <algorithm>
#include <sstream>
#include <string>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/asio/streambuf.hpp>

#include "ssf/layer/proxy/base64.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

std::string Base64::Encode(const std::string& input) {
  return Encode(Buffer(input.begin(), input.end()));
}

std::string Base64::Encode(const Base64::Buffer& input) {
  using namespace boost::archive::iterators;
  using Base64EncodeIterator =
      base64_from_binary<transform_width<Buffer::const_iterator, 6, 8> >;

  std::stringstream ss_encoded_input;

  std::copy(Base64EncodeIterator(input.begin()),
            Base64EncodeIterator(input.end()),
            ostream_iterator<char>(ss_encoded_input));

  // pad with '='
  switch (input.size() % 3) {
    case 1:
      ss_encoded_input << "==";
      break;
    case 2:
      ss_encoded_input << '=';
      break;
    default:
      break;
  }

  return ss_encoded_input.str();
}

Base64::Buffer Base64::Decode(const std::string& input) {
  using namespace boost::archive::iterators;
  using Base64DecodeIterator =
      transform_width<boost::archive::iterators::binary_from_base64<
                          remove_whitespace<const char*> >,
                      8, 6>;

  if (input.empty()) {
    return Buffer();
  }

  // compute padding size
  std::size_t padding_size = 0;
  while (input[input.size() - 1 - padding_size] == '=') {
    ++padding_size;
  }

  Buffer buf(Base64DecodeIterator(input.c_str()),
             Base64DecodeIterator(input.c_str() + input.size()));

  // remove padding
  buf.resize(buf.size() - padding_size);

  return buf;
}

}  // detail
}  // proxy
}  // layer
}  // ssf