#include <ctime>

#include <algorithm>
#include <random>
#include <set>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <openssl/md5.h>

#include "ssf/layer/proxy/digest_auth_strategy.h"

namespace ssf {
namespace layer {
namespace proxy {

DigestAuthStrategy::DigestAuthStrategy(const HttpProxy& proxy_ctx)
    : AuthStrategy(proxy_ctx, Status::kAuthenticating),
      request_populated_(false),
      qop_(Qop::kNone),
      cnonce_(GenerateRandomString(32)),
      nonce_count_(0) {}

std::string DigestAuthStrategy::AuthName() const { return "Digest"; }

bool DigestAuthStrategy::Support(const HttpResponse& response) const {
  return !request_populated_ && response.IsAuthenticationAllowed(AuthName());
}

void DigestAuthStrategy::ProcessResponse(const HttpResponse& response) {
  if (response.Success()) {
    status_ = Status::kAuthenticated;
    return;
  }

  if (!Support(response)) {
    status_ = Status::kAuthenticationFailure;
    return;
  }

  set_proxy_authentication(response);

  ParseDigestChallenge(response);

  if (status_ != Status::kAuthenticating) {
    return;
  }

  // mandatory challenge fields
  if (challenge_.count("realm") == 0 || challenge_.count("nonce") == 0) {
    status_ = Status::kAuthenticationFailure;
    return;
  }

  // extract qop type
  if (challenge_.count("qop")) {
    std::set<std::string> qops;
    boost::split(qops, challenge_["qop"], boost::is_any_of(","));
    if (qops.count("auth")) {
      qop_ = Qop::kAuth;
    } else if (qops.count("auth-int")) {
      qop_ = Qop::kAuthInt;
    }
  }
}

void DigestAuthStrategy::PopulateRequest(HttpRequest* p_request) {
  ++nonce_count_;

  // construct challenge response fields
  std::map<std::string, std::string> chal_resp;
  chal_resp["username"] = '"' + proxy_ctx_.username + '"';
  chal_resp["realm"] = '"' + challenge_["realm"] + '"';
  chal_resp["nonce"] = '"' + challenge_["nonce"] + '"';
  chal_resp["uri"] = '"' + p_request->uri() + '"';
  chal_resp["response"] = '"' + GenerateResponseDigest(*p_request) + '"';
  if (qop_ != Qop::kNone) {
    chal_resp["cnonce"] = '"' + cnonce_ + '"';
    chal_resp["nc"] = (boost::format("%08x") % nonce_count_).str();
    chal_resp["qop"] = ((qop_ == Qop::kAuth) ? "auth" : "auth-int");
  }
  if (challenge_.count("algorithm")) {
    if (boost::iequals(challenge_["algorithm"], "md5-sess")) {
      chal_resp["algorithm"] = challenge_["algorithm"];
    } else {
      chal_resp["algorithm"] = "MD5";
    }
  }
  if (challenge_.count("opaque")) {
    chal_resp["opaque"] = '"' + challenge_["opaque"] + '"';
  }

  // construct header
  std::stringstream ss_auth_hdr;
  ss_auth_hdr << AuthName() << " ";
  auto cur_it = chal_resp.begin();
  auto end_it = chal_resp.end();
  while (cur_it != end_it) {
    ss_auth_hdr << cur_it->first << '=' << cur_it->second;
    ++cur_it;
    if (cur_it != end_it) {
      ss_auth_hdr << ", ";
    }
  }

  p_request->AddHeader(
      proxy_authentication() ? "Proxy-Authorization" : "Authorization",
      ss_auth_hdr.str());

  request_populated_ = true;
}

static void eat_lws(const std::string& str, std::string::size_type& pos) {
  while (pos < str.length() && (str[pos] == ' ' || str[pos] == '\t')) {
    pos++;
  }
}

static void eat_quoted(const std::string& str, std::string::size_type& pos) {
  while (pos < str.length() && str[pos] != '\"') {
    pos++;
  }
}

static void eat_token(const std::string& str, std::string::size_type& pos) {
  while (pos < str.length() &&
         str[pos] != '(' &&
         str[pos] != ')' &&
         str[pos] != '<' &&
         str[pos] != '>' &&
         str[pos] != '@' &&
         str[pos] != ',' &&
         str[pos] != ';' &&
         str[pos] != ':' &&
         str[pos] != '\\' &&
         str[pos] != '"' &&
         str[pos] != '/' &&
         str[pos] != '[' &&
         str[pos] != ']' &&
         str[pos] != '?' &&
         str[pos] != '=' &&
         str[pos] != '{' &&
         str[pos] != '}' &&
         str[pos] != '=' &&
         str[pos] != ' ' &&
         str[pos] != '\t') {
    pos++;
  }
}

static std::string lower(const std::string& str) {
  std::string result(str);
  std::string::size_type index = 0;

  while (index < result.length()) {
    result[index] = std::tolower(result[index]);
    index++;
  }

  return result;
}

void DigestAuthStrategy::ParseDigestChallenge(const HttpResponse& response) {
  const std::string& challenge_str = ExtractAuthToken(response);
  std::string token, value;

  std::string::size_type i = 0;
  while (i < challenge_str.length()) {
    std::string::size_type s = i;

    eat_token(challenge_str, i);

    token = lower({challenge_str, s, i - s});

    eat_lws(challenge_str, i);

    if (challenge_str[i++] != '=') {
      status_ = Status::kAuthenticationFailure;
      return;
    }

    eat_lws(challenge_str, i);

    if (challenge_str[i] == '"' ) {
      s = ++i;
      eat_quoted(challenge_str, i);
      if (i < challenge_str.length() &&
          challenge_str[i++] != '"') {
        status_ = Status::kAuthenticationFailure;
        return;
      }
      value = {challenge_str, s, (i - 1) - s};
    } else {
      s = i;
      eat_token(challenge_str, i);
      value = {challenge_str, s, i - s};
    }

    eat_lws(challenge_str, i);
    if (i < challenge_str.length() &&
        challenge_str[i++] != ',') {
      status_ = Status::kAuthenticationFailure;
      return;
    }

    eat_lws(challenge_str, i);

    challenge_[token] = value;
  }
}

std::string DigestAuthStrategy::GenerateResponseDigest(
    const HttpRequest& request) {
  auto a1_hash = GenerateA1Hash();
  auto a2_hash = GenerateA2Hash(request);

  Md5Digest response_digest = {{0}};
  MD5_CTX md5_context;
  MD5_Init(&md5_context);

  // md5 a1_hash:nonce:
  MD5_Update(&md5_context, a1_hash.c_str(), a1_hash.size());
  MD5_Update(&md5_context, ":", 1);
  MD5_Update(&md5_context, challenge_["nonce"].c_str(),
             challenge_["nonce"].size());
  MD5_Update(&md5_context, ":", 1);

  if (qop_ != Qop::kNone) {
    auto nonce_str = (boost::format("%08x") % nonce_count_).str();
    std::string qop_str = ((qop_ == Qop::kAuth) ? "auth" : "auth-int");

    // nonce:cnonce:qop
    MD5_Update(&md5_context, nonce_str.c_str(), nonce_str.size());
    MD5_Update(&md5_context, ":", 1);
    MD5_Update(&md5_context, cnonce_.c_str(), cnonce_.size());
    MD5_Update(&md5_context, ":", 1);
    MD5_Update(&md5_context, qop_str.c_str(), qop_str.size());
    MD5_Update(&md5_context, ":", 1);
  }
  // a2_hash
  MD5_Update(&md5_context, a2_hash.c_str(), a2_hash.size());
  MD5_Final(response_digest.data(), &md5_context);

  return BufferToHex(response_digest.data(), response_digest.size());
}

std::string DigestAuthStrategy::GenerateA1Hash() {
  std::stringstream ss_a1;
  auto alg_it = challenge_.find("algorithm");

  Md5Digest a1_hash = {{0}};
  // md5 of username:realm:password
  MD5_CTX md5_context;
  MD5_Init(&md5_context);
  MD5_Update(&md5_context, proxy_ctx_.username.c_str(),
             proxy_ctx_.username.size());
  MD5_Update(&md5_context, ":", 1);
  MD5_Update(&md5_context, challenge_["realm"].c_str(),
             challenge_["realm"].size());
  MD5_Update(&md5_context, ":", 1);
  MD5_Update(&md5_context, proxy_ctx_.password.c_str(),
             proxy_ctx_.password.size());
  MD5_Final(a1_hash.data(), &md5_context);

  if (alg_it != challenge_.end() &&
      boost::iequals(alg_it->second, "md5-sess")) {
    // MD5-sess : a1_hash:nonce:cnonce
    MD5_Init(&md5_context);
    MD5_Update(&md5_context, a1_hash.data(), a1_hash.size());
    MD5_Update(&md5_context, ":", 1);
    MD5_Update(&md5_context, challenge_["nonce"].c_str(),
               challenge_["nonce"].size());
    MD5_Update(&md5_context, ":", 1);
    MD5_Update(&md5_context, cnonce_.c_str(), cnonce_.size());
    MD5_Final(a1_hash.data(), &md5_context);
  }

  return BufferToHex(a1_hash.data(), a1_hash.size());
}

std::string DigestAuthStrategy::GenerateA2Hash(const HttpRequest& request) {
  std::stringstream ss_a2;

  Md5Digest a2_hash = {{0}};
  MD5_CTX md5_context;
  MD5_Init(&md5_context);

  // md5 method:uri
  MD5_Update(&md5_context, request.method().c_str(), request.method().size());
  MD5_Update(&md5_context, ":", 1);
  MD5_Update(&md5_context, request.uri().c_str(), request.uri().size());

  if (qop_ == Qop::kAuthInt) {
    // include request body in md5
    MD5_Update(&md5_context, ":", 1);
    MD5_Update(&md5_context, request.body().c_str(), request.body().size());
  }
  MD5_Final(a2_hash.data(), &md5_context);

  return BufferToHex(a2_hash.data(), a2_hash.size());
}

std::string DigestAuthStrategy::GenerateRandomString(std::size_t strlen) {
  std::string random_string(strlen, '0');
  std::string characters(
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "1234567890");
  random_string.reserve(strlen);

  // TODO improve randomness
  std::random_device rd;
  std::mt19937 rng(rd());
  std::uniform_int_distribution<> index_dist(
      0, static_cast<int>(characters.size() - 1));
  for (std::size_t i = 0; i < strlen; ++i) {
    random_string[i] = characters[index_dist(rng)];
  }

  return random_string;
}

std::string DigestAuthStrategy::BufferToHex(unsigned char* buffer,
                                            std::size_t buffer_len) {
  std::string buffer_hex(buffer_len * 2, '0');
  unsigned char j;

  for (std::size_t i = 0; i < buffer_len; ++i) {
    j = (buffer[i] >> 4) & 0xf;
    buffer_hex[i * 2] = (j <= 9) ? (j + '0') : (j + 'a' - 10);

    j = buffer[i] & 0xf;
    buffer_hex[(i * 2) + 1] = (j <= 9) ? (j + '0') : (j + 'a' - 10);
  }

  return buffer_hex;
}

}  // proxy
}  // layer
}  // ssf
