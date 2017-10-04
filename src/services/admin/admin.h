#ifndef SSF_SERVICES_ADMIN_ADMIN_H_
#define SSF_SERVICES_ADMIN_ADMIN_H_

#include <vector>
#include <string>
#include <functional>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/thread/recursive_mutex.hpp>

#include <boost/system/error_code.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include <ssf/network/manager.h>

#include "common/boost/fiber/stream_fiber.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"

#include "services/base_service.h"

#include "services/admin/requests/create_service_request.h"
#include "services/admin/admin_command.h"
#include "services/admin/requests/stop_service_request.h"

#include "core/factory_manager/service_factory_manager.h"
#include "core/factories/service_factory.h"

#include "services/user_services/base_user_service.h"

namespace ssf {
namespace services {
namespace admin {

template <typename Demux>
class Admin : public BaseService<Demux> {
 public:
  using BaseUserServicePtr =
      typename ssf::services::BaseUserService<Demux>::BaseUserServicePtr;
  using AdminCallbackType =
      std::function<void(BaseUserServicePtr, const boost::system::error_code&)>;

 private:
  using LocalPortType = typename Demux::local_port_type;
  using AdminPtr = std::shared_ptr<Admin>;
  using ServiceManager =
      ItemManager<typename ssf::BaseService<Demux>::BaseServicePtr>;

  using Parameters = typename ssf::BaseService<Demux>::Parameters;
  using FiberAcceptor = typename ssf::BaseService<Demux>::fiber_acceptor;
  using Fiber = typename ssf::BaseService<Demux>::fiber;
  using FiberEndpoint = typename ssf::BaseService<Demux>::endpoint;

  using CommandHandler = std::function<void(const boost::system::error_code&)>;
  using IdToCommandHandlerMap = std::map<uint32_t, CommandHandler>;

 public:
  static AdminPtr Create(boost::asio::io_service& io_service,
                         Demux& fiber_demux, Parameters parameters) {
    return AdminPtr(new Admin(io_service, fiber_demux));
  }

  ~Admin() { SSF_LOG(kLogDebug) << "service[admin]: destroy"; }

  enum {
    kFactoryId = 1,
    kServicePort = (1 << 17) + 1,  // first of the service range
    kKeepAliveInterval = 120,      // seconds
    kServiceStatusRetryCount = 50  // retries
  };

  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<Demux>> p_factory) {
    p_factory->RegisterServiceCreator(kFactoryId, &Admin::Create);
  }

  void set_server();
  void SetClient(std::vector<BaseUserServicePtr> user_services,
                 AdminCallbackType callback);

  template <typename Request, typename Handler>
  void Command(Request request, Handler handler) {
    std::string parameters_buff_to_send = request.OnSending();

    auto serial = GetAvailableSerial();
    InsertHandler(serial, handler);

    auto p_command = std::make_shared<AdminCommand>(
        serial, request.command_id, (uint32_t)parameters_buff_to_send.size(),
        parameters_buff_to_send);

    auto do_handler =
        [p_command](const boost::system::error_code& ec, size_t length) {};

    async_SendCommand(*p_command, do_handler);
  }

  void InsertHandler(uint32_t serial, CommandHandler command_handler) {
    boost::recursive_mutex::scoped_lock lock1(command_handlers_mutex_);
    command_handlers_[serial] = command_handler;
  }

  // execute handler bound to the command serial id if exists
  void ExecuteAndRemoveCommandHandler(uint32_t serial) {
    if (command_handlers_.count(command_serial_received_)) {
      this->get_io_service().post(
          boost::bind(command_handlers_[command_serial_received_],
                      boost::system::error_code()));
      this->EraseHandler(command_serial_received_);
    }
  }

  void EraseHandler(uint32_t serial) {
    boost::recursive_mutex::scoped_lock lock1(command_handlers_mutex_);
    command_handlers_.erase(serial);
  }

  uint32_t GetAvailableSerial() {
    boost::recursive_mutex::scoped_lock lock1(command_handlers_mutex_);

    for (uint32_t serial = 3; serial < std::numeric_limits<uint32_t>::max();
         ++serial) {
      if (command_handlers_.count(serial + !!is_server_) == 0) {
        command_handlers_[serial + !!is_server_] =
            [](const boost::system::error_code&) {};
        return serial + !!is_server_;
      }
    }
    return 0;
  }

 public:
  // BaseService
  void start(boost::system::error_code& ec);
  void stop(boost::system::error_code& ec);
  uint32_t service_type_id();

 private:
  Admin(boost::asio::io_service& io_service, Demux& fiber_demux);

  void AsyncAccept();
  void FiberAcceptHandler(const boost::system::error_code& ec);

  void AsyncConnect();
  void FiberConnectHandler(const boost::system::error_code& ec);

  void HandleStop();

  void Initialize();
  void StartRemoteService(
      const admin::CreateServiceRequest<Demux>& create_request,
      const CommandHandler& handler);
  void StopRemoteService(const admin::StopServiceRequest<Demux>& stop_request,
                         const CommandHandler& handler);
  void InitializeRemoteServices(const boost::system::error_code& ec);
  void ListenForCommand();
  void DoAdmin(
      const boost::system::error_code& ec = boost::system::error_code(),
      size_t length = 0);
  void PostKeepAlive(const boost::system::error_code& ec, size_t length);
  void SendKeepAlive(const boost::system::error_code& ec);
  void ReceiveInstructionHeader();
  void ReceiveInstructionParameters();
  void ProcessInstructionId();
  void ShutdownServices();

  template <typename Handler>
  void async_SendCommand(const AdminCommand& command, Handler handler) {
    auto do_handler = [handler](const boost::system::error_code& ec,
                                size_t length) { handler(ec, length); };

    boost::asio::async_write(fiber_, command.const_buffers(), do_handler);
  }

  void Notify(BaseUserServicePtr p_user_service, boost::system::error_code ec) {
    if (callback_) {
      this->get_io_service().post(
          boost::bind(callback_, p_user_service, std::move(ec)));
    }
  }

  AdminPtr SelfFromThis() {
    return std::static_pointer_cast<Admin>(this->shared_from_this());
  }

 private:
  // Version information
  uint8_t admin_version_;

  // For initiating fiber connection
  bool is_server_;

  FiberAcceptor fiber_acceptor_;
  Fiber fiber_;

  // The status in which the admin service is
  uint32_t status_;

  // Buffers to receive commands
  uint32_t command_serial_received_;
  uint32_t command_id_received_;
  uint32_t command_size_received_;
  std::vector<char> parameters_buff_received_;

  // Keep alives
  uint32_t reserved_keep_alive_id_;
  uint32_t reserved_keep_alive_size_;
  std::string reserved_keep_alive_parameters_;
  boost::asio::steady_timer reserved_keep_alive_timer_;

  // List of user services
  std::vector<BaseUserServicePtr> user_services_;

  // Connection attempts
  uint8_t retries_;

  // Is stopped?
  boost::recursive_mutex stopping_mutex_;
  bool stopped_;

  // Initialize services
  boost::asio::coroutine coroutine_;
  size_t i_;
  std::vector<admin::CreateServiceRequest<Demux>> create_request_vector_;
  size_t j_;
  uint32_t remote_all_started_;
  uint16_t init_retries_;
  std::vector<admin::StopServiceRequest<Demux>> stop_request_vector_;
  bool local_all_started_;
  boost::system::error_code init_ec_;

  boost::recursive_mutex command_handlers_mutex_;
  IdToCommandHandlerMap command_handlers_;

  AdminCallbackType callback_;
};

}  // admin
}  // services
}  // ssf

#include "services/admin/admin.ipp"

#endif  // SSF_SERVICES_ADMIN_ADMIN_H_
