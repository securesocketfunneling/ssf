#include <iostream>
#include <fstream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <gtest/gtest.h>

#include "common/config/config.h"

class LoadConfigTest : public ::testing::Test {
 protected:
  ssf::config::Config config_;
};

TEST_F(LoadConfigTest, LoadNoFileTest) {
  boost::system::error_code ec;
  config_.Update("", ec);
  ASSERT_EQ(ec.value(), 0) << "Success if no file given";
}

TEST_F(LoadConfigTest, LoadNonExistantFileTest) {
  boost::system::error_code ec;
  config_.Update("./config_files/unknown.json", ec);
  ASSERT_NE(ec.value(), 0) << "No success if file not existant";
}

TEST_F(LoadConfigTest, LoadEmptyFileTest) {
  boost::system::error_code ec;

  config_.Update("./config_files/empty.json", ec);
  ASSERT_NE(ec.value(), 0) << "No success if file empty";
}

TEST_F(LoadConfigTest, LoadWrongFormatFileTest) {
  boost::system::error_code ec;

  config_.Update("./config_files/wrong_format.json", ec);
  ASSERT_NE(ec.value(), 0) << "No success if wrong file format";
}

TEST_F(LoadConfigTest, LoadTlsPartialFileTest) {
  boost::system::error_code ec;

  config_.Update("./config_files/tls_partial.json", ec);
  ASSERT_EQ(ec.value(), 0) << "Success if partial file format";
  ASSERT_EQ(config_.tls.ca_cert_path, "test_ca_path");
  ASSERT_EQ(config_.tls.cert_path, "./certs/certificate.crt");
  ASSERT_EQ(config_.tls.key_path, "test_key_path");
  ASSERT_EQ(config_.tls.key_password, "");
  ASSERT_EQ(config_.tls.dh_path, "test_dh_path");
  ASSERT_EQ(config_.tls.cipher_alg, "DHE-RSA-AES256-GCM-SHA384");
}

TEST_F(LoadConfigTest, LoadTlsCompleteFileTest) {
  boost::system::error_code ec;

  config_.Update("./config_files/tls_complete.json", ec);
  ASSERT_EQ(ec.value(), 0) << "Success if complete file format";
  ASSERT_EQ(config_.tls.ca_cert_path, "test_ca_path");
  ASSERT_EQ(config_.tls.cert_path, "test_cert_path");
  ASSERT_EQ(config_.tls.key_path, "test_key_path");
  ASSERT_EQ(config_.tls.key_password, "test_key_password");
  ASSERT_EQ(config_.tls.dh_path, "test_dh_path");
  ASSERT_EQ(config_.tls.cipher_alg, "test_cipher_alg");
  ASSERT_EQ(config_.proxy.http_addr, "");
  ASSERT_EQ(config_.proxy.http_port, "");
}

TEST_F(LoadConfigTest, LoadProxyFileTest) {
  boost::system::error_code ec;

  config_.Update("./config_files/proxy.json", ec);
  ASSERT_EQ(ec.value(), 0) << "Success if complete file format";
  ASSERT_EQ(config_.proxy.http_addr, "127.0.0.1");
  ASSERT_EQ(config_.proxy.http_port, "8080");
}

TEST_F(LoadConfigTest, LoadCompleteFileTest) {
  boost::system::error_code ec;

  config_.Update("./config_files/complete.json", ec);
  ASSERT_EQ(ec.value(), 0) << "Success if complete file format";
  ASSERT_EQ(config_.tls.ca_cert_path, "test_ca_path");
  ASSERT_EQ(config_.tls.cert_path, "test_cert_path");
  ASSERT_EQ(config_.tls.key_path, "test_key_path");
  ASSERT_EQ(config_.tls.key_password, "test_key_password");
  ASSERT_EQ(config_.tls.dh_path, "test_dh_path");
  ASSERT_EQ(config_.tls.cipher_alg, "test_cipher_alg");
}
