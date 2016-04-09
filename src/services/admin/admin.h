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
#include <boost/asio.hpp>  // NOLINT

#include <ssf/network/manager.h>

#include "common/boost/fiber/stream_fiber.hpp"
#include "common/boost/fiber/basic_fiber_demux.hpp"

#include "services/initialisation.h"
#include "services/base_service.h"

#include "services/admin/requests/create_service_request.h"
#include "services/admin/admin_command.h"
#include "services/admin/requests/stop_service_request.h"

#include "core/factory_manager/service_factory_manager.h"
#include "core/factories/service_factory.h"

#include "services/user_services/base_user_service.h"

namespace ssf { namespace services { namespace admin {

template <typename Demux>
class Admin : public BaseService<Demux> {
public:
 typedef typename ssf::services::BaseUserService<Demux>::BaseUserServicePtr
     BaseUserServicePtr;
 typedef std::function<void(
     ssf::services::initialisation::type, BaseUserServicePtr,
     const boost::system::error_code&)> AdminCallbackType;

 private:
  typedef typename Demux::local_port_type local_port_type;

  typedef std::shared_ptr<Admin> AdminPtr;
  typedef ItemManager<typename ssf::BaseService<Demux>::BaseServicePtr>
      ServiceManager;

  typedef typename ssf::BaseService<Demux>::Parameters Parameters;
  typedef typename ssf::BaseService<Demux>::demux demux;
  typedef typename ssf::BaseService<Demux>::endpoint endpoint;

  typedef std::function<void(const boost::system::error_code&)> CommandHandler;
  typedef std::map<uint32_t, CommandHandler> IdToCommandHandlerMap;

 public:
  static AdminPtr Create(boost::asio::io_service& io_service,
                         Demux& fiber_demux, Parameters parameters) {
    return std::shared_ptr<Admin>(new Admin(io_service, fiber_demux));
  }

  ~Admin() {}

  enum {
    factory_id = 1,
    service_port = (1 << 17) + 1,  // first of the service range
    keep_alive_interval = 120,  // seconds
    service_status_retry_interval = 100,  // milliseconds
    service_status_retry_number = 500  // retries
  };

  static void RegisterToServiceFactory(
    std::shared_ptr<ServiceFactory<Demux>> p_factory) {
    p_factory->RegisterServiceCreator(factory_id, &Admin::Create);
  }

  void set_server();
  void set_client(
      std::vector<BaseUserServicePtr>,
      AdminCallbackType callback);

  virtual void start(boost::system::error_code& ec);
  virtual void stop(boost::system::error_code& ec);
  virtual uint32_t service_type_id();

  template <typename Request, typename Handler>
  void Command(Request request,
               Handler handler) {
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
      this->get_io_service().post(boost::bind(
          command_handlers_[command_serial_received_], boost::system::error_code()));
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

 private:
  Admin(boost::asio::io_service& io_service, Demux& fiber_demux);

  void StartAccept();
  void HandleAccept(const boost::system::error_code& ec);
  void StartConnect();
  void HandleConnect(const boost::system::error_code& ec);
  void HandleStop();
  void Initialize();
  void StartRemoteService(
      const admin::CreateServiceRequest<demux>& create_request,
      const CommandHandler& handler);
  void StopRemoteService(const admin::StopServiceRequest<demux>& stop_request,
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
  void TreatInstructionId();
  void CreateNewService();
  void ShutdownServices();

  template <typename Handler>
  void async_SendCommand(const AdminCommand& command, Handler handler) {
    auto do_handler = [handler](const boost::system::error_code& ec,
                                size_t length) { handler(ec, length); };

    boost::asio::async_write(fiber_, command.const_buffers(), do_handler);
  }

  void Notify(ssf::services::initialisation::type type,
              BaseUserServicePtr p_user_service, boost::system::error_code ec) {
    if (callback_) {
      this->get_io_service().post(boost::bind(callback_, std::move(type),
                                              p_user_service, std::move(ec)));
    }
  }

  template <typename Handler, typename This>
  auto Then(Handler handler,
            This me) -> decltype(boost::bind(handler, me->SelfFromThis(), _1)) {
    return boost::bind(handler, me->SelfFromThis(), _1);
  }

  std::shared_ptr<Admin> SelfFromThis() {
    return std::static_pointer_cast<Admin>(this->shared_from_this());
  }

 private:
  // Version information
  uint8_t admin_version_;

  // For initiating fiber connection
  bool is_server_;
  typename ssf::BaseService<Demux>::fiber_acceptor fiber_acceptor_;
  typename ssf::BaseService<Demux>::fiber fiber_;

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
  boost::asio::deadline_timer reserved_keep_alive_timer_;

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
  std::vector<admin::CreateServiceRequest<demux>> create_request_vector_;
  size_t j_;
  uint32_t remote_all_started_;
  uint16_t init_retries_;
  std::vector<admin::StopServiceRequest<demux>> stop_request_vector_;
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
