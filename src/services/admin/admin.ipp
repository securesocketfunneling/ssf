#ifndef SSF_SERVICES_ADMIN_ADMIN_IPP_
#define SSF_SERVICES_ADMIN_ADMIN_IPP_

#include <chrono>
#include <memory>

#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "common/error/error.h"

namespace ssf {
namespace services {
namespace admin {

template <typename Demux>
Admin<Demux>::Admin(boost::asio::io_service& io_service, Demux& fiber_demux)
    : ssf::BaseService<Demux>::BaseService(io_service, fiber_demux),
      admin_version_(1),
      is_server_(false),
      fiber_acceptor_(io_service),
      fiber_(io_service),
      reserved_keep_alive_id_(0),
      reserved_keep_alive_size_(0),
      reserved_keep_alive_parameters_(),
      reserved_keep_alive_timer_(io_service),
      retries_(0),
      stopping_mutex_(),
      stopped_(false),
      callback_() {}

template <typename Demux>
void Admin<Demux>::SetAsServer() {
  is_server_ = true;
  FiberEndpoint ep(this->get_demux(), kServicePort);
  fiber_acceptor_.bind(ep, init_ec_);
  fiber_acceptor_.listen(boost::asio::socket_base::max_connections, init_ec_);
}

template <typename Demux>
void Admin<Demux>::SetAsClient(std::vector<BaseUserServicePtr> user_services,
                               AdminCallbackType callback) {
  is_server_ = false;
  user_services_ = std::move(user_services);
  callback_ = std::move(callback);
}

template <typename Demux>
void Admin<Demux>::start(boost::system::error_code& ec) {
  ec = init_ec_;

  if (ec) {
    return;
  }

  if (is_server_) {
    AsyncAccept();
  } else {
    AsyncConnect();
  }
}

template <typename Demux>
void Admin<Demux>::stop(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "service[admin]: stopping";
  ec.assign(::error::success, ::error::get_ssf_category());

  HandleStop();
}

template <typename Demux>
uint32_t Admin<Demux>::service_type_id() {
  return kFactoryId;
}

template <typename Demux>
void Admin<Demux>::AsyncAccept() {
  auto self = this->shared_from_this();
  auto on_fiber_accept = [this, self](const boost::system::error_code& ec) {
    OnFiberAccept(ec);
  };
  fiber_acceptor_.async_accept(fiber_, std::move(on_fiber_accept));
}

template <typename Demux>
void Admin<Demux>::OnFiberAccept(const boost::system::error_code& ec) {
  if (!fiber_acceptor_.is_open()) {
    return;
  }

  if (ec) {
    SSF_LOG(kLogError) << "service[admin]: error accepting new connection: "
                       << ec << " " << ec.value();
    ShutdownServices();
    return;
  }

  Initialize();
}

template <typename Demux>
void Admin<Demux>::AsyncConnect() {
  auto self = this->shared_from_this();
  FiberEndpoint ep(this->get_demux(), kServicePort);
  auto on_fiber_connect = [this, self](const boost::system::error_code& ec) {
    OnFiberConnect(ec);
  };
  fiber_.async_connect(ep, std::move(on_fiber_connect));
}

template <typename Demux>
void Admin<Demux>::OnFiberConnect(const boost::system::error_code& ec) {
  if (!fiber_.is_open() || ec) {
    SSF_LOG(kLogError) << "service[admin]: no new connection: " << ec.message()
                       << " " << ec.value();
    // Retry to connect if failed to open the fiber
    if (retries_ < kServiceStatusRetryCount) {
      this->AsyncConnect();
      ++retries_;
    }
    return;
  }

  this->Initialize();
  this->InitializeRemoteServices(boost::system::error_code());
}

template <typename Demux>
void Admin<Demux>::Initialize() {
  // Initialize the command reception processes
  this->ListenForCommand();

  // Initialize the keep alive processes
  this->PostKeepAlive(boost::system::error_code(), 0);
}

#include <boost/asio/yield.hpp>  // NOLINT

/// Initialize micro services (local and remote) asked by the client
template <typename Demux>
void Admin<Demux>::InitializeRemoteServices(
    const boost::system::error_code& ec) {
  if (ec) {
    SSF_LOG(kLogDebug) << "service[admin]: ec intializing " << ec.value();
    return;
  }

  auto self = this->shared_from_this();

  reenter(coroutine_) {
    // For each user service
    for (i_ = 0; i_ < user_services_.size(); ++i_) {
      // Get the remote micro services to start
      create_request_vector_ =
          user_services_[i_]->GetRemoteServiceCreateVector();

      // For each remote micro service to start
      for (j_ = 0; j_ < create_request_vector_.size(); ++j_) {
        // Start remove service and yield until server response comes back
        yield StartRemoteService(
            create_request_vector_[j_],
            [this, self](const boost::system::error_code& ec) {
              InitializeRemoteServices(ec);
            });
      }

      // At this point, all remote services have responded with their statuses
      remote_all_started_ =
          user_services_[i_]->CheckRemoteServiceStatus(this->get_demux());

      // If something went wrong remote_all_started_ > 0
      if (remote_all_started_) {
        SSF_LOG(kLogError) << "service[admin]: could not start remote "
                              "microservice for service["
                           << user_services_[i_]->GetName() << "]";

        Notify(user_services_[i_],
               boost::system::error_code(::error::operation_canceled,
                                         ::error::get_ssf_category()));

        // Get the remote micro services to stop
        stop_request_vector_ =
            user_services_[i_]->GetRemoteServiceStopVector(this->get_demux());
        for (j_ = 0; j_ < stop_request_vector_.size(); ++j_) {
          // Send a service stop request
          yield StopRemoteService(
              stop_request_vector_[j_],
              [this, self](const boost::system::error_code& ec) {
                InitializeRemoteServices(ec);
              });
        }
        // Try next user service
        continue;
      }

      // Start local associated services
      local_all_started_ =
          user_services_[i_]->StartLocalServices(this->get_demux());

      // If something went wrong local_all_started_ == false
      if (!local_all_started_) {
        SSF_LOG(kLogError) << "service[admin]: could not start local "
                              "microservice for service["
                           << user_services_[i_]->GetName() << "]";

        Notify(user_services_[i_],
               boost::system::error_code(::error::operation_canceled,
                                         ::error::get_ssf_category()));

        // Get the remote micro services to stop
        stop_request_vector_ =
            user_services_[i_]->GetRemoteServiceStopVector(this->get_demux());
        for (j_ = 0; j_ < stop_request_vector_.size(); ++j_) {
          // Send a service stop request
          yield StopRemoteService(
              stop_request_vector_[j_],
              [this, self](const boost::system::error_code& ec) {
                InitializeRemoteServices(ec);
              });
        }
        // Stop local services
        user_services_[i_]->StopLocalServices(this->get_demux());
        // Try next user service
        continue;
      }

      Notify(user_services_[i_],
             boost::system::error_code(::error::success,
                                       ::error::get_ssf_category()));
    }
  }
}
#include <boost/asio/unyield.hpp>  // NOLINT

template <typename Demux>
void Admin<Demux>::ListenForCommand() {
  status_ = 101;
  auto self = SelfFromThis();
  auto do_admin = [this, self]() { DoAdmin(boost::system::error_code(), 0); };
  this->get_io_service().post(do_admin);
}

template <typename Demux>
void Admin<Demux>::DoAdmin(const boost::system::error_code& ec, size_t length) {
  if (ec) {
    auto self = this->shared_from_this();
    auto shutdown_services = [this, self]() { ShutdownServices(); };
    this->get_io_service().post(shutdown_services);
    return;
  }

  switch (status_) {
    case 101:
      this->ReceiveInstructionHeader();
      break;

    case 102:
      if (command_id_received_) {
        this->ReceiveInstructionParameters();
      } else {
        this->ListenForCommand();
      }
      break;

    case 103:
      this->ProcessInstructionId();
      break;

    default:
      /*get_io_service().post(std::bind(&Admin::DoAdmin,
                                          this, this->shared_from_this(),
                                          boost::system::error_code(),
                                          0));*/
      break;
  }
}

template <typename Demux>
void Admin<Demux>::ReceiveInstructionHeader() {
  status_ = 102;  // Receive current instruction parameter size

  std::array<boost::asio::mutable_buffer, 3> buff = {
      {boost::asio::buffer(&command_serial_received_,
                           sizeof(command_serial_received_)),
       boost::asio::buffer(&command_id_received_, sizeof(command_id_received_)),
       boost::asio::buffer(&command_size_received_,
                           sizeof(command_size_received_))}};

  // receive the command header
  auto self = this->shared_from_this();
  auto on_fiber_read = [this, self](const boost::system::error_code& ec,
                                    std::size_t length) {
    DoAdmin(ec, length);
  };
  boost::asio::async_read(fiber_, buff, std::move(on_fiber_read));
}

template <typename Demux>
void Admin<Demux>::ReceiveInstructionParameters() {
  status_ = 103;  // Handler current instruction

  // @todo check size
  parameters_buff_received_.resize(command_size_received_);

  // Receive the command parameters
  auto self = this->shared_from_this();
  auto on_fiber_read = [this, self](const boost::system::error_code& ec,
                                    std::size_t length) {
    DoAdmin(ec, length);
  };
  boost::asio::async_read(fiber_,
                          boost::asio::buffer(parameters_buff_received_),
                          std::move(on_fiber_read));
}

/// Execute the command received and send back the result
template <typename Demux>
void Admin<Demux>::ProcessInstructionId() {
  // Get the right function to execute the command
  auto p_executer = cmd_factory_.GetExecuter(command_id_received_);

  const std::string serialized(&parameters_buff_received_[0],
                               parameters_buff_received_.size());

  Demux& d = this->get_demux();

  // @todo Add a not found error ?
  if (p_executer) {
    // Execute and get the result
    boost::system::error_code ec;
    std::string serialized_result = (*p_executer)(serialized, &d, ec);

    // Get the right function to reply
    auto p_replier = cmd_factory_.GetReplier(command_id_received_);

    // Get the reply
    std::string reply = (*p_replier)(serialized, &d, ec, serialized_result);

    // If there is something to send back
    if (reply.size() > 0) {
      uint32_t* p_reply_command_index =
          cmd_factory_.GetReplyCommandIndex(command_id_received_);

      // reply with command serial received (command handler execution)
      auto p_command = std::make_shared<AdminCommand>(
          command_serial_received_, *p_reply_command_index,
          (uint32_t)reply.size(), reply);

      this->AsyncSendCommand(
          *p_command, [p_command](const boost::system::error_code&, size_t) {});
    }
  }

  // Execute an handler bound to the serial command (e.g, callback after a
  // service started)
  this->ExecuteAndRemoveCommandHandler(command_serial_received_);

  this->ListenForCommand();
}

template <typename Demux>
void Admin<Demux>::StartRemoteService(
    const admin::CreateServiceRequest<Demux>& create_request,
    const CommandHandler& handler) {
  this->Command(create_request, handler);
}

template <typename Demux>
void Admin<Demux>::StopRemoteService(
    const admin::StopServiceRequest<Demux>& stop_request,
    const CommandHandler& handler) {
  this->Command(stop_request, handler);
}

template <typename Demux>
void Admin<Demux>::OnSendKeepAlive(const boost::system::error_code& ec) {
  if (ec) {
    auto self = this->shared_from_this();
    auto shutdown_services = [this, self]() { ShutdownServices(); };
    this->get_io_service().post(shutdown_services);
    return;
  }

  auto p_command = std::make_shared<AdminCommand>(
      0, reserved_keep_alive_id_, reserved_keep_alive_size_,
      reserved_keep_alive_parameters_);

  auto self = this->shared_from_this();
  auto on_command_sent = [this, self, p_command](
      const boost::system::error_code& ec, size_t length) {
    this->PostKeepAlive(ec, length);
  };

  this->AsyncSendCommand(*p_command, on_command_sent);
}

template <typename Demux>
void Admin<Demux>::PostKeepAlive(const boost::system::error_code& ec,
                                 size_t length) {
  auto self = this->shared_from_this();
  if (ec) {
    auto shutdown_services = [this, self]() { ShutdownServices(); };
    this->get_io_service().post(shutdown_services);
    return;
  }

  reserved_keep_alive_timer_.expires_from_now(
      std::chrono::seconds(kKeepAliveInterval));
  auto on_timeout = [this, self](const boost::system::error_code& ec) {
    OnSendKeepAlive(ec);
  };
  reserved_keep_alive_timer_.async_wait(on_timeout);
}

template <typename Demux>
void Admin<Demux>::ShutdownServices() {
  std::unique_lock<std::recursive_mutex> lock(stopping_mutex_);
  if (stopped_) {
    return;
  }

  Demux& d = this->get_demux();
  auto p_service_factory = ServiceFactoryManager<Demux>::GetServiceFactory(&d);
  if (p_service_factory) {
    p_service_factory->Destroy();
  }
  callback_ = [](BaseUserServicePtr, const boost::system::error_code&) {};
  stopped_ = true;
}

template <typename Demux>
void Admin<Demux>::HandleStop() {
  reserved_keep_alive_timer_.cancel();
  fiber_acceptor_.close();
  fiber_.close();
}

}  // admin
}  // services
}  // ssf

#endif  // SSF_SERVICES_ADMIN_ADMIN_IPP_
