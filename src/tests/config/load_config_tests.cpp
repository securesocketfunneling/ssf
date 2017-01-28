#include <iostream>
#include <fstream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <gtest/gtest.h>

#include "common/config/config.h"

class LoadConfigTest : public ::testing::Test {
 public:
  virtual void SetUp() { config_.Init(); }

 protected:
  ssf::config::Config config_;
};

TEST_F(LoadConfigTest, LoadNoFileTest) {
  boost::system::error_code ec;

  config_.UpdateFromFile("", ec);

  ASSERT_EQ(ec.value(), 0) << "Success if no file given";
}

TEST_F(LoadConfigTest, LoadNonExistantFileTest) {
  boost::system::error_code ec;

  config_.UpdateFromFile("./config_files/unknown.json", ec);

  ASSERT_NE(ec.value(), 0) << "No success if file not existant";
}

TEST_F(LoadConfigTest, LoadEmptyFileTest) {
  boost::system::error_code ec;

  config_.UpdateFromFile("./config_files/empty.json", ec);

  ASSERT_NE(ec.value(), 0) << "No success if file empty";
}

TEST_F(LoadConfigTest, LoadWrongFormatFileTest) {
  boost::system::error_code ec;

  config_.UpdateFromFile("./config_files/wrong_format.json", ec);

  ASSERT_NE(ec.value(), 0) << "No success if wrong file format";
}

TEST_F(LoadConfigTest, DefaultValueTest) {
  ASSERT_EQ(config_.tls().ca_cert_path(), "./certs/trusted/ca.crt");
  ASSERT_EQ(config_.tls().cert_path(), "./certs/certificate.crt");
  ASSERT_EQ(config_.tls().key_path(), "./certs/private.key");
  ASSERT_EQ(config_.tls().key_password(), "");
  ASSERT_EQ(config_.tls().dh_path(), "./certs/dh4096.pem");
  ASSERT_EQ(config_.tls().cipher_alg(), "DHE-RSA-AES256-GCM-SHA384");

  ASSERT_EQ(config_.http_proxy().host(), "");
  ASSERT_EQ(config_.http_proxy().port(), "");
  ASSERT_EQ(config_.http_proxy().username(), "");
  ASSERT_EQ(config_.http_proxy().domain(), "");
  ASSERT_EQ(config_.http_proxy().password(), "");
  ASSERT_EQ(config_.http_proxy().reuse_kerb(), true);
  ASSERT_EQ(config_.http_proxy().reuse_ntlm(), true);

  ASSERT_EQ(config_.socks_proxy().version(), ssf::network::Socks::Version::kV5);
  ASSERT_EQ(config_.socks_proxy().host(), "");
  ASSERT_EQ(config_.socks_proxy().port(), "1080");

  ASSERT_TRUE(config_.services().datagram_forwarder().enabled());
  ASSERT_TRUE(config_.services().datagram_listener().enabled());
  ASSERT_FALSE(config_.services().datagram_listener().gateway_ports());
  ASSERT_FALSE(config_.services().file_copy().enabled());
  ASSERT_TRUE(config_.services().socks().enabled());
  ASSERT_TRUE(config_.services().stream_forwarder().enabled());
  ASSERT_TRUE(config_.services().stream_listener().enabled());
  ASSERT_FALSE(config_.services().stream_listener().gateway_ports());
  ASSERT_FALSE(config_.services().process().enabled());

  ASSERT_GT(config_.services().process().path().length(),
            static_cast<std::size_t>(0));
  ASSERT_EQ(config_.services().process().args(), "");
}

TEST_F(LoadConfigTest, LoadTlsPartialFileTest) {
  boost::system::error_code ec;

  config_.UpdateFromFile("./config_files/tls_partial.json", ec);

  ASSERT_EQ(ec.value(), 0) << "Success if partial file format";
  ASSERT_EQ(config_.tls().ca_cert_path(), "test_ca_path");
  ASSERT_EQ(config_.tls().cert_path(), "./certs/certificate.crt");
  ASSERT_EQ(config_.tls().key_path(), "test_key_path");
  ASSERT_EQ(config_.tls().key_password(), "");
  ASSERT_EQ(config_.tls().dh_path(), "test_dh_path");
  ASSERT_EQ(config_.tls().cipher_alg(), "DHE-RSA-AES256-GCM-SHA384");
}

TEST_F(LoadConfigTest, LoadTlsCompleteFileTest) {
  boost::system::error_code ec;

  config_.UpdateFromFile("./config_files/tls_complete.json", ec);

  ASSERT_EQ(ec.value(), 0) << "Success if complete file format";
  ASSERT_EQ(config_.tls().ca_cert_path(), "test_ca_path");
  ASSERT_EQ(config_.tls().cert_path(), "test_cert_path");
  ASSERT_EQ(config_.tls().key_path(), "test_key_path");
  ASSERT_EQ(config_.tls().key_password(), "test_key_password");
  ASSERT_EQ(config_.tls().dh_path(), "test_dh_path");
  ASSERT_EQ(config_.tls().cipher_alg(), "test_cipher_alg");
  ASSERT_EQ(config_.http_proxy().host(), "");
  ASSERT_EQ(config_.http_proxy().port(), "");
  ASSERT_EQ(config_.http_proxy().username(), "");
  ASSERT_EQ(config_.http_proxy().domain(), "");
  ASSERT_EQ(config_.http_proxy().password(), "");
  ASSERT_EQ(config_.http_proxy().reuse_kerb(), true);
  ASSERT_EQ(config_.http_proxy().reuse_ntlm(), true);

  ASSERT_TRUE(config_.services().datagram_forwarder().enabled());
  ASSERT_TRUE(config_.services().datagram_listener().enabled());
  ASSERT_FALSE(config_.services().datagram_listener().gateway_ports());
  ASSERT_FALSE(config_.services().file_copy().enabled());
  ASSERT_TRUE(config_.services().socks().enabled());
  ASSERT_TRUE(config_.services().stream_forwarder().enabled());
  ASSERT_TRUE(config_.services().stream_listener().enabled());
  ASSERT_FALSE(config_.services().stream_listener().gateway_ports());
  ASSERT_FALSE(config_.services().process().enabled());

  ASSERT_GT(config_.services().process().path().length(),
            static_cast<std::size_t>(0));
  ASSERT_EQ(config_.services().process().args(), "");
}

TEST_F(LoadConfigTest, LoadProxyFileTest) {
  boost::system::error_code ec;

  config_.UpdateFromFile("./config_files/proxy.json", ec);
  ASSERT_EQ(ec.value(), 0) << "Success if complete file format";
  ASSERT_EQ(config_.http_proxy().host(), "127.0.0.1");
  ASSERT_EQ(config_.http_proxy().port(), "8080");
  ASSERT_EQ(config_.http_proxy().username(), "test_user");
  ASSERT_EQ(config_.http_proxy().domain(), "test_domain");
  ASSERT_EQ(config_.http_proxy().password(), "test_password");
  ASSERT_EQ(config_.http_proxy().reuse_ntlm(), false);
  ASSERT_EQ(config_.http_proxy().reuse_kerb(), false);

  ASSERT_EQ(config_.socks_proxy().version(),
            ssf::network::Socks::Version::kV5);
  ASSERT_EQ(config_.socks_proxy().host(), "127.0.0.2");
  ASSERT_EQ(config_.socks_proxy().port(), "1080");
}

TEST_F(LoadConfigTest, LoadServicesFileTest) {
  boost::system::error_code ec;

  config_.UpdateFromFile("./config_files/services.json", ec);

  ASSERT_EQ(ec.value(), 0) << "Success if complete file format";
  ASSERT_FALSE(config_.services().datagram_forwarder().enabled());
  ASSERT_FALSE(config_.services().datagram_listener().enabled());
  ASSERT_TRUE(config_.services().datagram_listener().gateway_ports());
  ASSERT_TRUE(config_.services().file_copy().enabled());
  ASSERT_FALSE(config_.services().socks().enabled());
  ASSERT_FALSE(config_.services().stream_forwarder().enabled());
  ASSERT_FALSE(config_.services().stream_listener().enabled());
  ASSERT_TRUE(config_.services().stream_listener().gateway_ports());
  ASSERT_TRUE(config_.services().process().enabled());

  ASSERT_EQ(config_.services().process().path(), "/bin/custom_path");
  ASSERT_EQ(config_.services().process().args(), "-custom args");
}

TEST_F(LoadConfigTest, LoadCompleteFileTest) {
  boost::system::error_code ec;

  config_.UpdateFromFile("./config_files/complete.json", ec);

  ASSERT_EQ(ec.value(), 0) << "Success if complete file format";
  ASSERT_EQ(config_.tls().ca_cert_path(), "test_ca_path");
  ASSERT_EQ(config_.tls().cert_path(), "test_cert_path");
  ASSERT_EQ(config_.tls().key_path(), "test_key_path");
  ASSERT_EQ(config_.tls().key_password(), "test_key_password");
  ASSERT_EQ(config_.tls().dh_path(), "test_dh_path");
  ASSERT_EQ(config_.tls().cipher_alg(), "test_cipher_alg");

  ASSERT_EQ(config_.http_proxy().host(), "127.0.0.1");
  ASSERT_EQ(config_.http_proxy().port(), "8080");
  ASSERT_EQ(config_.http_proxy().username(), "test_user");
  ASSERT_EQ(config_.http_proxy().password(), "test_password");
  ASSERT_EQ(config_.http_proxy().domain(), "test_domain");
  ASSERT_EQ(config_.http_proxy().reuse_ntlm(), false);
  ASSERT_EQ(config_.http_proxy().reuse_kerb(), false);
  
  ASSERT_EQ(config_.socks_proxy().version(),
            ssf::network::Socks::Version::kV4);
  ASSERT_EQ(config_.socks_proxy().host(), "127.0.0.2");
  ASSERT_EQ(config_.socks_proxy().port(), "1080");

  ASSERT_EQ(config_.services().process().path(), "/bin/custom_path");
  ASSERT_EQ(config_.services().process().args(), "-custom args");
}
