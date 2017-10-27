#include "ssf_fixture_test.h"

#include "tests/tls_config_helper.h"

SSFFixtureTest::SSFFixtureTest() : success_(false), stopped_(false) {}

void SSFFixtureTest::SetUp() {
  ssf::log::Log::SetSeverityLevel(ssf::log::LogLevel::kLogInfo);
  StartAsyncEngine();
}

void SSFFixtureTest::TearDown() { StopAsyncEngine(); }

boost::asio::io_service& SSFFixtureTest::get_io_service() {
  return async_engine_.get_io_service();
}

void SSFFixtureTest::StartAsyncEngine() { async_engine_.Start(); }

void SSFFixtureTest::StopAsyncEngine() { async_engine_.Stop(); }

void SSFFixtureTest::StartClient(const std::string& server_port,
                                 ClientCallback callback,
                                 boost::system::error_code& ec) {
  std::vector<UserServicePtr> client_options;

  ssf::config::Config ssf_config;

  ssf_config.Init();
  ssf::tests::SetClientTlsConfig(&ssf_config);

  auto endpoint_query = NetworkProtocol::GenerateClientQuery(
      "127.0.0.1", server_port, ssf_config, {});

  auto on_user_service_status =
      [](UserServicePtr p_user_service, const boost::system::error_code& ec) {};
  p_ssf_client_.reset(new Client());
  p_ssf_client_->Init(endpoint_query, 1, 0, false, {}, ssf_config.services(),
                      callback, on_user_service_status, ec);
  if (ec) {
    return;
  }

  p_ssf_client_->Run(ec);
  if (ec) {
    SSF_LOG(kLogError) << "Could not run client";
    return;
  }
}

void SSFFixtureTest::StopClient() {
  if (!p_ssf_client_.get()) {
    return;
  }
  boost::system::error_code stop_ec;
  p_ssf_client_->Stop(stop_ec);
  p_ssf_client_->Deinit();
}

void SSFFixtureTest::StartServer(const std::string& addr,
                                 const std::string& server_port,
                                 boost::system::error_code& ec) {
  ssf::config::Config ssf_config;

  ssf_config.Init();
  ssf::tests::SetServerTlsConfig(&ssf_config);

  auto endpoint_query =
      NetworkProtocol::GenerateServerQuery(addr, server_port, ssf_config);
  p_ssf_server_.reset(new Server(ssf_config.services()));

  p_ssf_server_->Run(endpoint_query, ec);
}

void SSFFixtureTest::StopServer() {
  if (!p_ssf_server_.get()) {
    return;
  }
  p_ssf_server_->Stop();
}

void SSFFixtureTest::StartTimer(const std::chrono::seconds& wait_duration,
                                TimerCallback callback,
                                boost::system::error_code& ec) {
  p_timer_.reset(new boost::asio::steady_timer(async_engine_.get_io_service()));
  p_timer_->expires_from_now(wait_duration, ec);
  p_timer_->async_wait(callback);
}

void SSFFixtureTest::StopTimer() {
  if (!p_timer_.get()) {
    return;
  }
  boost::system::error_code ec;
  p_timer_->cancel(ec);
}

void SSFFixtureTest::WaitNotification() {
  std::unique_lock<std::mutex> lock(mutex_);
  wait_stop_cv_.wait(lock, [this]() { return stopped_; });
  lock.unlock();
}

void SSFFixtureTest::SendNotification(bool success) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stopped_ = true;
    success_ = success;
  }
  wait_stop_cv_.notify_all();
}

bool SSFFixtureTest::IsNotificationSuccess() { return success_; }
