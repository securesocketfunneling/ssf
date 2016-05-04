#include <iostream>
#include <fstream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <gtest/gtest.h>

#include "common/config/config.h"

class LoadConfigTest : public ::testing::Test {
 protected:
  LoadConfigTest() : filename_("config.json") {}

  ~LoadConfigTest() {}

  virtual void TearDown() {
    if (boost::filesystem::exists(filename_)) {
      std::remove(filename_.c_str());
    }
  }

  ssf::config::Config config_;
  std::string filename_;
};

//-----------------------------------------------------------------------------
TEST_F(LoadConfigTest, loadNoFile) {
  boost::system::error_code ec;
  config_.Update("", ec);
  ASSERT_EQ(ec.value(), 0) << "Success if no file given";
}

//-----------------------------------------------------------------------------
TEST_F(LoadConfigTest, loadNonExistantFile) {
  boost::system::error_code ec;
  config_.Update(filename_, ec);
  ASSERT_NE(ec.value(), 0) << "No success if file not existant";
}

//-----------------------------------------------------------------------------
TEST_F(LoadConfigTest, loadEmptyFile) {
  boost::system::error_code ec;
  std::ofstream file;
  file.open(filename_);
  file.close();
  config_.Update(filename_, ec);
  ASSERT_NE(ec.value(), 0) << "No success if file empty";
}

//-----------------------------------------------------------------------------
TEST_F(LoadConfigTest, loadWrongFormatFile) {
  boost::system::error_code ec;
  std::ofstream file;
  file.open(filename_);
  file << "{ \"ssf\": { \"tls\" : { \"ca_cert_path\": \"test\" } }";
  file.close();
  config_.Update(filename_, ec);
  std::remove(filename_.c_str());
  ASSERT_NE(ec.value(), 0) << "No success if wrong file format";
}

//-----------------------------------------------------------------------------
TEST_F(LoadConfigTest, loadPartialFile) {
  boost::system::error_code ec;
  std::ofstream file;
  file.open(filename_);
  file << "{ \"ssf\": { \"tls\" : { " << std::endl;
  file << "\"ca_cert_path\": \"test_ca_path\"," << std::endl;
  file << "\"key_path\": \"test_key_path\"," << std::endl;
  file << "\"dh_path\": \"test_dh_path\"" << std::endl;
  file << "} } }";
  file.close();

  config_.Update(filename_, ec);
  ASSERT_EQ(ec.value(), 0) << "Success if partial file format";
  ASSERT_EQ(config_.tls.ca_cert_path, "test_ca_path");
  ASSERT_EQ(config_.tls.cert_path, "./certs/certificate.crt");
  ASSERT_EQ(config_.tls.key_path, "test_key_path");
  ASSERT_EQ(config_.tls.key_password, "");
  ASSERT_EQ(config_.tls.dh_path, "test_dh_path");
  ASSERT_EQ(config_.tls.cipher_alg, "DHE-RSA-AES256-GCM-SHA384");
}

//-----------------------------------------------------------------------------
TEST_F(LoadConfigTest, loadCompleteFile) {
  boost::system::error_code ec;
  std::ofstream file;
  file.open(filename_);
  file << "{ \"ssf\": { \"tls\" : { " << std::endl;
  file << "\"ca_cert_path\": \"test_ca_path\"," << std::endl;
  file << "\"cert_path\": \"test_cert_path\"," << std::endl;
  file << "\"key_path\": \"test_key_path\"," << std::endl;
  file << "\"key_password\": \"test_key_password\"," << std::endl;
  file << "\"dh_path\": \"test_dh_path\"," << std::endl;
  file << "\"cipher_alg\": \"test_cipher_alg\"" << std::endl;
  file << "} } }";
  file.close();

  config_.Update(filename_, ec);
  ASSERT_EQ(ec.value(), 0) << "Success if complete file format";
  ASSERT_EQ(config_.tls.ca_cert_path, "test_ca_path");
  ASSERT_EQ(config_.tls.cert_path, "test_cert_path");
  ASSERT_EQ(config_.tls.key_path, "test_key_path");
  ASSERT_EQ(config_.tls.key_password, "test_key_password");
  ASSERT_EQ(config_.tls.dh_path, "test_dh_path");
  ASSERT_EQ(config_.tls.cipher_alg, "test_cipher_alg");
}
