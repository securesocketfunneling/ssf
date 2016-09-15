#include "tests/network/ssf_fixture_test.h"

SSFFixtureTest::SSFFixtureTest() : success_(false), stopped_(false) {}

void SSFFixtureTest::SetUp() { StartAsyncEngine(); }

void SSFFixtureTest::TearDown() { StopAsyncEngine(); }

boost::asio::io_service& SSFFixtureTest::get_io_service() {
  return async_engine_.get_io_service();
}

void SSFFixtureTest::StartAsyncEngine() { async_engine_.Start(); }

void SSFFixtureTest::StopAsyncEngine() { async_engine_.Stop(); }

void SSFFixtureTest::StartClient(const std::string& server_port,
                                 ClientCallback callback,
                                 boost::system::error_code& ec) {
  std::vector<BaseUserServicePtr> client_options;

  ssf::config::Config ssf_config;

  ssf_config.Init();

  auto endpoint_query = NetworkProtocol::GenerateClientQuery(
      "127.0.0.1", server_port, ssf_config, {});

  p_ssf_client_.reset(
      new Client(client_options, ssf_config.services(), callback));

  p_ssf_client_->Run(endpoint_query, ec);
}

void SSFFixtureTest::StopClient() {
  if (!p_ssf_client_.get()) {
    return;
  }
  p_ssf_client_->Stop();
}

void SSFFixtureTest::StartServer(const std::string& addr,
                                 const std::string& server_port,
                                 boost::system::error_code& ec) {
  ssf::config::Config ssf_config;

  ssf_config.Init();

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
    boost::lock_guard<std::mutex> lock(mutex_);
    stopped_ = true;
    success_ = success;
  }
  wait_stop_cv_.notify_all();
}

bool SSFFixtureTest::IsNotificationSuccess() { return success_; }