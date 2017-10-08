#include "tests/services/file_copy_fixture_test.h"

#include "tests/tls_config_helper.h"

FileCopyTestFixture::FileCopyTestFixture()
    : p_ssf_client_(nullptr), p_ssf_server_(nullptr) {}

FileCopyTestFixture::~FileCopyTestFixture() {}

void FileCopyTestFixture::SetUp() {
  ssf::log::Log::SetSeverityLevel(ssf::log::LogLevel::kLogDebug);
}

void FileCopyTestFixture::TearDown() {
  StopClientThreads();
  StopServerThreads();

  // Clean receiver path
  boost::filesystem::path receiver_path(GetOutputPattern());
  if (boost::filesystem::is_directory(receiver_path)) {
    for (boost::filesystem::directory_iterator end_it, it(receiver_path);
         it != end_it; ++it) {
      std::remove(it->path().string().c_str());
    }
    return;
  }

  if (boost::filesystem::is_regular_file(receiver_path)) {
    std::remove(GetOutputPattern().c_str());
  }
}

void FileCopyTestFixture::StartServer(const std::string& server_port) {
  boost::system::error_code ec;
  ssf::config::Config ssf_config;
  ssf_config.Init();
  ssf::tests::SetServerTlsConfig(&ssf_config);

  const char* new_config = R"RAWSTRING(
{
    "ssf": {
        "services" : {
            "file_copy": { "enable": true }
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

void FileCopyTestFixture::StartClient(const std::string& server_port) {
  boost::system::error_code ec;
  ssf::UserServiceParameters copy_parameters = {
      {CopyFileService::GetParseName(),
       {CopyFileService::CreateUserServiceParameters(
           GetFromStdin(), GetFromLocalToRemote(), GetInputPattern(),
           GetOutputPattern(), ec)}}};

  ssf::config::Config ssf_config;
  ssf_config.Init();
  ssf::tests::SetClientTlsConfig(&ssf_config);

  const char* new_config = R"RAWSTRING(
{
    "ssf": {
        "services" : {
            "file_copy": { "enable": true }
        }
    }
}
)RAWSTRING";

  ssf_config.UpdateFromString(new_config, ec);
  ASSERT_EQ(ec.value(), 0) << "Could not update server config from string "
                           << new_config;

  auto endpoint_query = NetworkProtocol::GenerateClientQuery(
      "127.0.0.1", server_port, ssf_config, {});
  p_ssf_client_.reset(new Client());
  p_ssf_client_->Register<CopyFileService>();
  p_ssf_client_->Init(
      endpoint_query, 1, 0, true, copy_parameters, ssf_config.services(),
      std::bind(&FileCopyTestFixture::OnClientStatus, this,
                std::placeholders::_1),
      std::bind(&FileCopyTestFixture::OnClientUserServiceStatus, this,
                std::placeholders::_1, std::placeholders::_2),
      ec);
  if (ec) {
    return;
  }

  p_ssf_client_->Run(ec);
  if (ec) {
    SSF_LOG(kLogError) << "Could not run client";
  }
  return;
}

bool FileCopyTestFixture::Wait() {
  auto network_set_future = network_set_.get_future();
  auto service_set_future = service_set_.get_future();
  auto transport_set_future = transport_set_.get_future();

  network_set_future.wait();
  service_set_future.wait();
  transport_set_future.wait();

  return network_set_future.get() && service_set_future.get() &&
         transport_set_future.get();
}

bool FileCopyTestFixture::WaitClose() {
  auto close_future = close_set_.get_future();
  boost::system::error_code ec;
  p_ssf_client_->WaitStop(ec);
  close_future.wait();
  return close_future.get();
}

std::string FileCopyTestFixture::GetOutputPattern() const {
  return "files_copied/";
}

void FileCopyTestFixture::StopServerThreads() { p_ssf_server_->Stop(); }

void FileCopyTestFixture::StopClientThreads() {
  boost::system::error_code stop_ec;
  p_ssf_client_->Stop(stop_ec);
}

void FileCopyTestFixture::OnClientStatus(ssf::Status status) {
  switch (status) {
    case ssf::Status::kEndpointNotResolvable:
    case ssf::Status::kServerUnreachable:
      SSF_LOG(kLogCritical) << "Network initialization failed";
      network_set_.set_value(false);
      transport_set_.set_value(false);
      service_set_.set_value(false);
      break;
    case ssf::Status::kServerNotSupported:
      SSF_LOG(kLogCritical) << "Transport initialization failed";
      transport_set_.set_value(false);
      service_set_.set_value(false);
      break;
    case ssf::Status::kConnected:
      network_set_.set_value(true);
      break;
    case ssf::Status::kDisconnected:
      SSF_LOG(kLogInfo) << "client: disconnected";
      close_set_.set_value(true);
      break;
    case ssf::Status::kRunning:
      transport_set_.set_value(true);
      break;
    default:
      break;
  }
}

void FileCopyTestFixture::OnClientUserServiceStatus(
    UserServicePtr p_user_service, const boost::system::error_code& ec) {
  if (ec) {
    SSF_LOG(kLogCritical) << "user_service[" << p_user_service->GetName()
                          << "]: initialization failed";
  }
  if (p_user_service->GetName() == CopyFileService::GetParseName()) {
    service_set_.set_value(!ec);
  }
}