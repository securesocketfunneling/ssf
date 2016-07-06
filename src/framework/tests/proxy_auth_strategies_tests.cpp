#include <gtest/gtest.h>

#include "ssf/layer/proxy/base64.h"
#include "ssf/layer/proxy/basic_auth_strategy.h"
#include "ssf/layer/proxy/digest_auth_strategy.h"

#include "ssf/layer/proxy/proxy_endpoint_context.h"

TEST(Base64Test, Test) {
  using Base64 = ssf::layer::proxy::detail::Base64;
  std::string str("This is a test string");
  Base64::Buffer buffer({'T', 'h', 'i', 's', ' ', 'i', 's', ' ', 'a', ' ', 't',
                         'e', 's', 't', ' ', 's', 't', 'r', 'i', 'n', 'g', '\0',
                         'C', 'u', 't', 't', 'e', 'd'});
  auto empty_encoded = Base64::Encode("");
  auto encoded_str = Base64::Encode(str);
  auto encoded_buf = Base64::Encode(buffer);

  ASSERT_EQ("VGhpcyBpcyBhIHRlc3Qgc3RyaW5n", encoded_str);
  ASSERT_EQ("VGhpcyBpcyBhIHRlc3Qgc3RyaW5nAEN1dHRlZA==", encoded_buf);

  auto empty_decoded = Base64::Decode(empty_encoded);
  auto decoded_str_buffer = Base64::Decode(encoded_str);
  auto decoded_buffer = Base64::Decode(encoded_buf);

  std::string decoded_str(decoded_str_buffer.begin(), decoded_str_buffer.end());

  ASSERT_EQ(0, empty_decoded.size());
  ASSERT_EQ(str, decoded_str);
  ASSERT_EQ(buffer, decoded_buffer);
}

TEST(ProxyAuthStrategiesTest, BasicAuthTest) {
  using BasicAuthStrategy = ssf::layer::proxy::detail::BasicAuthStrategy;
  using HttpRequest = ssf::layer::proxy::detail::HttpRequest;
  using HttpResponse = ssf::layer::proxy::detail::HttpResponse;
  using Proxy = ssf::layer::proxy::Proxy;

  Proxy proxy_ctx;
  proxy_ctx.username = "Aladdin";
  proxy_ctx.password = "open sesame";

  BasicAuthStrategy basic_auth(proxy_ctx);

  HttpResponse response;
  HttpRequest request("GET", "/dir/index.html");

  response.set_status_code(HttpResponse::kProxyAuthenticationRequired);
  response.AddHeader("WWW-Authenticate", "Basic realm=\"WallyWorld\"");

  ASSERT_NE(basic_auth.status(), BasicAuthStrategy::kAuthenticationFailure);
  ASSERT_TRUE(basic_auth.Support(response));

  basic_auth.ProcessResponse(response);
  ASSERT_NE(basic_auth.status(), BasicAuthStrategy::kAuthenticationFailure);

  basic_auth.PopulateRequest(&request);
  ASSERT_NE(basic_auth.status(), BasicAuthStrategy::kAuthenticationFailure);

  ASSERT_FALSE(basic_auth.Support(response));

  auto authorization_hdr = request.Header("Authorization");
  ASSERT_FALSE(authorization_hdr.empty());

  ASSERT_NE(authorization_hdr.find("QWxhZGRpbjpvcGVuIHNlc2FtZQ=="),
            std::string::npos)
      << "credentials not found";
}

TEST(ProxyAuthStrategiesTest, DigestAuthTest) {
  using DigestAuthStrategy = ssf::layer::proxy::detail::DigestAuthStrategy;
  using HttpRequest = ssf::layer::proxy::detail::HttpRequest;
  using HttpResponse = ssf::layer::proxy::detail::HttpResponse;
  using Proxy = ssf::layer::proxy::Proxy;

  Proxy proxy_ctx;
  proxy_ctx.username = "Mufasa";
  proxy_ctx.password = "Circle Of Life";

  DigestAuthStrategy digest_auth(proxy_ctx);
  digest_auth.set_cnonce("0a4f113b");

  HttpResponse response;
  HttpRequest request("GET", "/dir/index.html");

  response.set_status_code(HttpResponse::kProxyAuthenticationRequired);
  response.AddHeader("WWW-Authenticate",
                     "Digest realm =\"testrealm@host.com\", qop"
                     "=\"auth,auth-int\", nonce "
                     "=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", opaque "
                     "=\"5ccc069c403ebaf9f0171e9517f40e41\"");

  ASSERT_NE(digest_auth.status(), DigestAuthStrategy::kAuthenticationFailure);
  ASSERT_TRUE(digest_auth.Support(response));

  digest_auth.ProcessResponse(response);
  ASSERT_NE(digest_auth.status(), DigestAuthStrategy::kAuthenticationFailure);

  digest_auth.PopulateRequest(&request);
  ASSERT_NE(digest_auth.status(), DigestAuthStrategy::kAuthenticationFailure);

  ASSERT_FALSE(digest_auth.Support(response));

  auto authorization_hdr = request.Header("Authorization");
  ASSERT_FALSE(authorization_hdr.empty());

  ASSERT_NE(authorization_hdr.find("uri=\"/dir/index.html\""),
            std::string::npos)
      << "uri not found";
  ASSERT_NE(authorization_hdr.find("qop=auth"), std::string::npos)
      << "qop not found";
  ASSERT_NE(
      authorization_hdr.find("response=\"6629fae49393a05397450978507c4ef1\""),
      std::string::npos)
      << "response digest not found";
}

TEST(ProxyAuthStrategiesTest, NtlmAuthTest) {}

TEST(ProxyAuthStrategiesTest, NegotiateAuthTest) {}