#include "tests/services/file_copy_fixture_test.h"

FileCopyTestFixture::FileCopyTestFixture()
    : p_ssf_client_(nullptr), p_ssf_server_(nullptr) {}

FileCopyTestFixture::~FileCopyTestFixture() {}

void FileCopyTestFixture::SetUp() {
  StartServer();
  StartClient();
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

void FileCopyTestFixture::StartServer() {
  ssf::config::Config ssf_config;
  boost::system::error_code ec;

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
      NetworkProtocol::GenerateServerQuery("", "8000", ssf_config);

  p_ssf_server_.reset(new Server(ssf_config.services()));

  boost::system::error_code run_ec;
  p_ssf_server_->Run(endpoint_query, run_ec);
}

void FileCopyTestFixture::StartClient() {
  std::vector<BaseUserServicePtr> client_services;
  boost::system::error_code ec;
  auto p_service =
      ssf::services::CopyFileService<demux>::CreateServiceFromParams(
          GetFromStdin(), GetFromLocalToRemote(), GetInputPattern(),
          GetOutputPattern(), ec);

  client_services.push_back(p_service);

  ssf::config::Config ssf_config;

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
      NetworkProtocol::GenerateClientQuery("127.0.0.1", "8000", ssf_config, {});

  p_ssf_client_.reset(new Client(
      client_services, ssf_config.services(),
      boost::bind(&FileCopyTestFixture::SSFClientCallback, this, _1, _2, _3)));
  boost::system::error_code run_ec;
  p_ssf_client_->Run(endpoint_query, run_ec);
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
  close_future.wait();
  return close_future.get();
}

std::string FileCopyTestFixture::GetOutputPattern() const {
  return "files_copied/";
}

void FileCopyTestFixture::StopServerThreads() { p_ssf_server_->Stop(); }

void FileCopyTestFixture::StopClientThreads() { p_ssf_client_->Stop(); }

void FileCopyTestFixture::SSFClientCallback(
    ssf::services::initialisation::type type,
    FileCopyTestFixture::BaseUserServicePtr p_user_service,
    const boost::system::error_code& ec) {
  if (type == ssf::services::initialisation::NETWORK) {
    network_set_.set_value(!ec);
    if (ec) {
      service_set_.set_value(false);
      transport_set_.set_value(false);
    }
    return;
  }

  if (type == ssf::services::initialisation::TRANSPORT) {
    transport_set_.set_value(!ec);
    return;
  }

  if (type == ssf::services::initialisation::SERVICE &&
      p_user_service->GetName() == "copy_file") {
    service_set_.set_value(!ec);
    return;
  }

  if (type == ssf::services::initialisation::CLOSE) {
    close_set_.set_value(true);
    return;
  }
}