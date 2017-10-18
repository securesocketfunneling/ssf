#include "tests/services/copy_fixture_test.h"

#include <cstdlib>
#include <ctime>

#include "common/crypto/hash.h"
#include "common/crypto/sha256.h"
#include "common/filesystem/path.h"

#include "tests/tls_config_helper.h"

#include "services/copy/copy_context.h"

static constexpr char kInputDirectory[] = "files_to_copy";
static constexpr char kOutputDirectory[] = "files_copied";

CopyFixtureTest::CopyFixtureTest()
    : p_ssf_client_(nullptr), p_ssf_server_(nullptr) {}

void CopyFixtureTest::SetUp() {
  ssf::log::Log::SetSeverityLevel(ssf::log::LogLevel::kLogInfo);
  std::srand(static_cast<uint32_t>(std::time(0)));

  // clean generated files
  if (boost::filesystem::is_directory(GetInputDirectory().GetString())) {
    for (boost::filesystem::directory_iterator end_it,
         it(GetInputDirectory().GetString());
         it != end_it; ++it) {
      std::remove(it->path().string().c_str());
    }
  }
  if (boost::filesystem::is_directory(GetOutputDirectory().GetString())) {
    for (boost::filesystem::directory_iterator end_it,
         it(GetOutputDirectory().GetString());
         it != end_it; ++it) {
      std::remove(it->path().string().c_str());
    }
  }
}

void CopyFixtureTest::TearDown() {
  StopClientThreads();
  StopServerThreads();
}

void CopyFixtureTest::StartServer(const std::string& server_port) {
  boost::system::error_code ec;
  ssf::config::Config ssf_config;
  ssf_config.Init();
  ssf::tests::SetServerTlsConfig(&ssf_config);

  const char* new_config = R"RAWSTRING(
{
    "ssf": {
        "services" : {
            "copy": { "enable": true }
        }
    }
}
)RAWSTRING";

  ssf_config.UpdateFromString(new_config, ec);
  ASSERT_EQ(ec.value(), 0) << "Could not update server config from string "
                           << new_config;

  auto endpoint_query =
      NetworkProtocol::GenerateServerQuery("", server_port, ssf_config);

  p_ssf_server_.reset(new Server(ssf_config.services()));

  boost::system::error_code run_ec;
  p_ssf_server_->Run(endpoint_query, run_ec);
}

void CopyFixtureTest::StartClient(const std::string& server_port) {
  boost::system::error_code ec;
  ssf::UserServiceParameters copy_parameters = {
      {Copy::GetParseName(), {Copy::CreateUserServiceParameters(ec)}}};

  ssf::config::Config ssf_config;
  ssf_config.Init();
  ssf::tests::SetClientTlsConfig(&ssf_config);

  const char* new_config = R"RAWSTRING(
{
    "ssf": {
        "services" : {
            "copy": { "enable": true }
        }
    }
}
)RAWSTRING";

  ssf_config.UpdateFromString(new_config, ec);
  ASSERT_EQ(ec.value(), 0) << "could not update server config from string "
                           << new_config;

  auto endpoint_query = NetworkProtocol::GenerateClientQuery(
      "127.0.0.1", server_port, ssf_config, {});
  p_ssf_client_.reset(new Client());
  p_ssf_client_->Register<Copy>();
  p_ssf_client_->Init(
      endpoint_query, 1, 0, true, copy_parameters, ssf_config.services(),
      std::bind(&CopyFixtureTest::OnClientStatus, this, std::placeholders::_1),
      std::bind(&CopyFixtureTest::OnClientUserServiceStatus, this,
                std::placeholders::_1, std::placeholders::_2),
      ec);
  if (ec) {
    return;
  }

  p_ssf_client_->Run(ec);
  if (ec) {
    SSF_LOG(kLogError) << "[copy_tests] could not run client";
  }
  return;
}

bool CopyFixtureTest::Wait() {
  auto network_set_future = network_set_.get_future();
  auto service_set_future = service_set_.get_future();
  auto transport_set_future = transport_set_.get_future();

  network_set_future.wait();
  service_set_future.wait();
  transport_set_future.wait();

  return network_set_future.get() && service_set_future.get() &&
         transport_set_future.get();
}

bool CopyFixtureTest::StartCopy(const ssf::services::copy::CopyRequest& req,
                                bool from_client_to_server) {
  boost::system::error_code ec;

  ClientSessionPtr session = p_ssf_client_->GetSession(ec);
  if (ec) {
    SSF_LOG(kLogError) << "[copy_tests] cannot get client session";
    return false;
  }

  auto on_file_status = [this](ssf::services::copy::CopyContext* context,
                               const boost::system::error_code& ec) {};
  auto on_file_copied = [this](ssf::services::copy::CopyContext* context,
                               const boost::system::error_code& ec) {
    if (context->is_stdin_input) {
      SSF_LOG(kLogInfo) << "[copy_tests] stdin copied in "
                        << context->output_dir << " "
                        << context->output_filename << " " << ec.message();
    } else {
      SSF_LOG(kLogInfo) << "[copy_tests] file copied from "
                        << context->input_filepath << " to "
                        << context->GetOutputFilepath().GetString() << " "
                        << ec.message();
    }
  };
  auto on_copy_finished = [this](uint64_t files_count, uint64_t errors_count,
                                 const boost::system::error_code& ec) {
    SSF_LOG(kLogInfo) << "[copy_tests] copy finished ("
                      << (files_count - errors_count) << "/" << files_count
                      << " files copied) " << ec.message();
    boost::system::error_code stop_ec;
    p_ssf_client_->Stop(stop_ec);
  };
  copy_client_ = CopyClient::Create(session, on_file_status, on_file_copied,
                                    on_copy_finished, ec);
  if (ec) {
    SSF_LOG(kLogError) << "[copy_tests] cannot create copy client";
    return false;
  }

  if (from_client_to_server) {
    copy_client_->AsyncCopyToServer(req);
  } else {
    copy_client_->AsyncCopyFromServer(req);
  }

  return true;
}

bool CopyFixtureTest::WaitClose() {
  boost::system::error_code ec;
  auto close_future = close_set_.get_future();
  p_ssf_client_->WaitStop(ec);
  close_future.wait();
  return close_future.get();
}
void CopyFixtureTest::StopServerThreads() { p_ssf_server_->Stop(); }

void CopyFixtureTest::StopClientThreads() {
  boost::system::error_code stop_ec;
  p_ssf_client_->Stop(stop_ec);
}

void CopyFixtureTest::OnClientStatus(ssf::Status status) {
  switch (status) {
    case ssf::Status::kEndpointNotResolvable:
    case ssf::Status::kServerUnreachable:
      SSF_LOG(kLogCritical) << "[copy_tests] network initialization failed";
      network_set_.set_value(false);
      transport_set_.set_value(false);
      service_set_.set_value(false);
      break;
    case ssf::Status::kServerNotSupported:
      SSF_LOG(kLogCritical) << "[copy_tests] transport initialization failed";
      transport_set_.set_value(false);
      service_set_.set_value(false);
      break;
    case ssf::Status::kConnected:
      network_set_.set_value(true);
      break;
    case ssf::Status::kDisconnected:
      SSF_LOG(kLogInfo) << "[copy_tests] client disconnected";
      close_set_.set_value(true);
      break;
    case ssf::Status::kRunning:
      transport_set_.set_value(true);
      break;
    default:
      break;
  }
}

ssf::Path CopyFixtureTest::GetInputDirectory() { return kInputDirectory; }

ssf::Path CopyFixtureTest::GetOutputDirectory() { return kOutputDirectory; }

void CopyFixtureTest::OnClientUserServiceStatus(
    UserServicePtr p_user_service, const boost::system::error_code& ec) {
  if (ec) {
    SSF_LOG(kLogCritical) << "[copy_tests] user_service["
                          << p_user_service->GetName()
                          << "]: initialization failed";
  }
  if (p_user_service->GetName() == Copy::GetParseName()) {
    service_set_.set_value(!ec);
  }
}

ssf::Path CopyFixtureTest::GenerateRandomFile(const ssf::Path& file_dir,
                                              const std::string& name_prefix,
                                              const std::string& file_suffix,
                                              uint64_t filesize) {
  auto file_path = file_dir;
  file_path /= name_prefix;
  file_path += std::to_string(rand());
  file_path += file_suffix;

  std::ofstream file(file_path.GetString());

  for (uint64_t i = 0; i < filesize / sizeof(int); ++i) {
    int random = rand();
    file.write(reinterpret_cast<const char*>(&random), sizeof(int));
  }

  return file_path;
}

bool CopyFixtureTest::AreFilesEqual(const ssf::Path& source_filepath,
                                    const ssf::Path& test_filepath) {
  if (!boost::filesystem::is_regular_file(test_filepath.GetString())) {
    SSF_LOG(kLogError) << "[copy_tests] file " << test_filepath.GetString()
                       << " should exist in files_copied";
    return false;
  }

  boost::system::error_code ec;
  auto source_digest =
      ssf::crypto::HashFile<ssf::crypto::Sha256>(source_filepath, ec);
  if (ec) {
    SSF_LOG(kLogError) << "[copy_tests] could not generate source file digest";
    return false;
  }
  auto test_digest =
      ssf::crypto::HashFile<ssf::crypto::Sha256>(test_filepath, ec);
  if (ec) {
    SSF_LOG(kLogError) << "[copy_tests] could not generate test file digest";
    return false;
  }

  return std::equal(source_digest.begin(), source_digest.end(),
                    test_digest.begin());
}
