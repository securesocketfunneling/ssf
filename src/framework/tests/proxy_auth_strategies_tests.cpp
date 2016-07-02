#include <gtest/gtest.h>

#include "ssf/layer/proxy/basic_auth_strategy.h"
#include "ssf/layer/proxy/digest_auth_strategy.h"
#include "ssf/layer/proxy/proxy_endpoint_context.h"

TEST(ProxyAuthStrategiesTest, BasicAuthTest) {
  using BasicAuthStrategy = ssf::layer::proxy::detail::BasicAuthStrategy;
  using HttpRequest = ssf::layer::proxy::detail::HttpRequest;
  using HttpResponse = ssf::layer::proxy::detail::HttpResponse;
  using EndpointContext = ssf::layer::proxy::ProxyEndpointContext;

  EndpointContext ep_ctx;
  ep_ctx.http_proxy.username = "Aladdin";
  ep_ctx.http_proxy.password = "open sesame";

  BasicAuthStrategy basic_auth;

  HttpResponse response;
  HttpRequest request("GET", "/dir/index.html");

  response.set_status_code(HttpResponse::kProxyAuthenticationRequired);
  response.AddHeader("WWW-Authenticate", "Basic realm=\"WallyWorld\"");

  ASSERT_NE(basic_auth.status(), BasicAuthStrategy::kAuthenticationFailure);
  ASSERT_TRUE(basic_auth.Support(response));

  basic_auth.ProcessResponse(response);
  ASSERT_NE(basic_auth.status(), BasicAuthStrategy::kAuthenticationFailure);

  basic_auth.PopulateRequest(ep_ctx, &request);
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
  using EndpointContext = ssf::layer::proxy::ProxyEndpointContext;

  EndpointContext ep_ctx;
  ep_ctx.http_proxy.username = "Mufasa";
  ep_ctx.http_proxy.password = "Circle Of Life";

  DigestAuthStrategy digest_auth;
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

  digest_auth.PopulateRequest(ep_ctx, &request);
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