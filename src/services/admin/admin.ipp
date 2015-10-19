#ifndef SSF_SERVICES_ADMIN_ADMIN_IPP_
#define SSF_SERVICES_ADMIN_ADMIN_IPP_

#include <memory>

#include <boost/log/trivial.hpp>
#include <boost/thread.hpp>
#include <boost/system/error_code.hpp>

#include "common/error/error.h"
#include "core/factories/command_factory.h"

namespace ssf {
namespace services {
namespace admin {
//----------------------------------------------------------------------------
template <typename Demux>
Admin<Demux>::Admin(boost::asio::io_service& io_service, Demux& fiber_demux)
    : ssf::BaseService<Demux>::BaseService(io_service, fiber_demux),
      admin_version_(1),
      is_server_(0),
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

//-----------------------------------------------------------------------------
template <typename Demux>
void Admin<Demux>::set_server() {
  is_server_ = 1;
  endpoint ep(this->get_demux(), service_port);
  fiber_acceptor_.bind(ep, init_ec_);
  fiber_acceptor_.listen(boost::asio::socket_base::max_connections, init_ec_);
}

//-----------------------------------------------------------------------------
template <typename Demux>
void Admin<Demux>::set_client(
    std::vector<BaseUserServicePtr> user_services,
    AdminCallbackType callback) {
  user_services_ = std::move(user_services);
  callback_ = std::move(callback);
}

//-----------------------------------------------------------------------------
template <typename Demux>
void Admin<Demux>::start(boost::system::error_code& ec) {
  ec = init_ec_;

  if (!init_ec_) {
    if (is_server_) {
      StartAccept();
    } else {
      StartConnect();
    }
  }
}

//-----------------------------------------------------------------------------
template <typename Demux>
void Admin<Demux>::stop(boost::system::error_code& ec) {
  BOOST_LOG_TRIVIAL(info) << "service admin: stopping";
  ec.assign(ssf::error::success, ssf::error::get_ssf_category());

  HandleStop();
}

//-----------------------------------------------------------------------------
template <typename Demux>
uint32_t Admin<Demux>::service_type_id() {
  return factory_id;
}

//-----------------------------------------------------------------------------
template <typename Demux>
void Admin<Demux>::StartAccept() {
  fiber_acceptor_.async_accept(
      fiber_, Then(&Admin::HandleAccept, this->SelfFromThis()));
}

//-----------------------------------------------------------------------------
template <typename Demux>
void Admin<Demux>::HandleAccept(const boost::system::error_code& ec) {
  BOOST_LOG_TRIVIAL(trace) << "service admin: handleAccept";

  if (!fiber_acceptor_.is_open()) {
    return;
  }

  if (ec) {
    BOOST_LOG_TRIVIAL(error) << "service admin: error accepting new connection: " << ec << " "
                             << ec.value();
    ShutdownServices();
  } else {
    Initialize();
  }
}

//-----------------------------------------------------------------------------
template <typename Demux>
void Admin<Demux>::StartConnect() {
  fiber_.async_connect(endpoint(this->get_demux(), service_port),
                       Then(&Admin::HandleConnect, this->SelfFromThis()));
}

//-----------------------------------------------------------------------------
template <typename Demux>
void Admin<Demux>::HandleConnect(const boost::system::error_code& ec) {
  BOOST_LOG_TRIVIAL(trace) << "service admin: handle connect";

  if (!fiber_.is_open() || ec) {
    BOOST_LOG_TRIVIAL(error)
        << "service admin: no new connection: " << ec.message() << " "
        << ec.value();
    // Retry to connect if failed to open the fiber
    if (retries_ < 50) {
      this->StartConnect();
      ++retries_;
    }
    return;
  } else {
    this->Initialize();
    this->InitializeRemoteServices(boost::system::error_code());
  }
}

//-----------------------------------------------------------------------------
template <typename Demux>
void Admin<Demux>::Initialize() {
  // Initialize the command reception processes
  this->ListenForCommand();

  // Initialize the keep alive processes
  this->PostKeepAlive(boost::system::error_code(), 0);
}

//-----------------------------------------------------------------------------
#include <boost/asio/yield.hpp>  // NOLINT

/// Initialize micro services (local and remote) asked by the client
template <typename Demux>
void Admin<Demux>::InitializeRemoteServices(
    const boost::system::error_code& ec) {
  if (!ec) {
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
              boost::bind(&Admin::InitializeRemoteServices,
                          this->SelfFromThis(), _1));
        }

        // At this point, all remote services have responded with their statuses
        remote_all_started_ =
            user_services_[i_]->CheckRemoteServiceStatus(this->get_demux());

        // If something went wrong remote_all_started_ > 0
        if (remote_all_started_) {
          BOOST_LOG_TRIVIAL(warning) << "service admin: remote could not start";

          Notify(
            ssf::services::initialisation::SERVICE,
            user_services_[i_],
            boost::system::error_code(ssf::error::operation_canceled,
                                      ssf::error::get_ssf_category()));

          // Get the remote micro services to stop
          stop_request_vector_ =
              user_services_[i_]->GetRemoteServiceStopVector(this->get_demux());
          for (j_ = 0; j_ < stop_request_vector_.size(); ++j_) {
            // Send a service stop request
            yield StopRemoteService(
                stop_request_vector_[j_],
                boost::bind(&Admin::InitializeRemoteServices,
                            this->SelfFromThis(), _1));
          }

          return;
        }

        // Start local associated services
        local_all_started_ =
            user_services_[i_]->StartLocalServices(this->get_demux());

        // If something went wrong local_all_started_ == false
        if (!local_all_started_) {
          BOOST_LOG_TRIVIAL(warning) << "service admin: local could not start";

          Notify(
            ssf::services::initialisation::SERVICE,
            user_services_[i_],
            boost::system::error_code(ssf::error::operation_canceled,
                                      ssf::error::get_ssf_category()));

          // Get the remote micro services to stop
          stop_request_vector_ =
              user_services_[i_]->GetRemoteServiceStopVector(this->get_demux());
          for (j_ = 0; j_ < stop_request_vector_.size(); ++j_) {
            // Send a service stop request
            yield StopRemoteService(
                stop_request_vector_[j_],
                boost::bind(&Admin::InitializeRemoteServices,
                            this->SelfFromThis(), _1));
          }
          // Stop local services
          user_services_[i_]->StopLocalServices(this->get_demux());
          
          return;
        }

        Notify(
          ssf::services::initialisation::SERVICE,
          user_services_[i_],
          boost::system::error_code(ssf::error::success,
                                    ssf::error::get_ssf_category()));
      }
    }
  } else {
    BOOST_LOG_TRIVIAL(debug) << "service admin: ec intializing " << ec.value();
  }
}
#include <boost/asio/unyield.hpp>  // NOLINT

//-----------------------------------------------------------------------------
template <typename Demux>
void Admin<Demux>::ListenForCommand() {
  status_ = 101;
  this->get_io_service().post(boost::bind(&Admin::DoAdmin, this->SelfFromThis(),
                                          boost::system::error_code(), 0));
}

//-----------------------------------------------------------------------------
template <typename Demux>
void Admin<Demux>::DoAdmin(const boost::system::error_code& ec, size_t length) {
  switch (status_) {
    case 101:
      if (!ec) {
        this->ReceiveInstructionHeader();
      } else {
        this->get_io_service().post(
          boost::bind(&Admin<Demux>::ShutdownServices, SelfFromThis()));
        return;
      }
      break;

    case 102:
      if (!ec) {
        if (command_id_received_) {
          this->ReceiveInstructionParameters();
        } else {
          this->ListenForCommand();
        }
      } else {
        this->get_io_service().post(
          boost::bind(&Admin<Demux>::ShutdownServices, SelfFromThis()));
        return;
      }
      break;

    case 103:
      if (!ec) {
        this->TreatInstructionId();
      } else {
        this->get_io_service().post(
          boost::bind(&Admin<Demux>::ShutdownServices, SelfFromThis()));
        return;
      }
      break;

    default:
      /*get_io_service().post(boost::bind(&Admin::DoAdmin,
                                          SelfFromThis(),
                                          boost::system::error_code(),
                                          0));*/
      break;
  }
}

//-----------------------------------------------------------------------------
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
  boost::asio::async_read(fiber_, buff,
                          boost::bind(&Admin::DoAdmin, SelfFromThis(), _1, _2));
}

//-----------------------------------------------------------------------------
template <typename Demux>
void Admin<Demux>::ReceiveInstructionParameters() {
  status_ = 103;  // Handler current instruction

  // @todo check size
  parameters_buff_received_.resize(command_size_received_);

  // Receive the command parameters
  boost::asio::async_read(
      fiber_, boost::asio::buffer(parameters_buff_received_),
      boost::bind(&Admin::DoAdmin, this->SelfFromThis(), _1, _2));
}

/// Execute the command received and send back the result
template <typename Demux>
void Admin<Demux>::TreatInstructionId() {

  // Get the right function to execute the command
  using CommandExecuterType =
      typename ssf::CommandFactory<Demux>::CommandExecuterType;
  CommandExecuterType* p_executer =
      ssf::CommandFactory<Demux>::GetExecuter(command_id_received_);

  const std::string serialized(&parameters_buff_received_[0],
                               parameters_buff_received_.size());
  std::istringstream istrs(serialized);
  boost::archive::text_iarchive ar(istrs);

  Demux& d = this->get_demux();

  // @todo Add a not found error ?
  if (p_executer) {
    // Execute and get the result
    boost::system::error_code ec;
    std::string serialized_result = (*p_executer)(ar, &d, ec);

    // Get the right function to reply
    using CommandReplierType =
        typename ssf::CommandFactory<Demux>::CommandReplierType;
    CommandReplierType* p_replier =
        ssf::CommandFactory<Demux>::GetReplier(command_id_received_);

    std::istringstream istrs2(serialized);
    boost::archive::text_iarchive ar2(istrs2);

    // Get the reply
    std::string reply = (*p_replier)(ar2, &d, ec, serialized_result);

    // If there is something to send back
    if (reply.size() > 0) {
      uint32_t* p_reply_command_index =
          ssf::CommandFactory<Demux>::GetReplyCommandIndex(
              command_id_received_);

      // reply with command serial received (command handler execution)
      auto p_command = std::make_shared<AdminCommand>(command_serial_received_,
                                                      *p_reply_command_index,
                                                      (uint32_t)reply.size(), reply);

      this->async_SendCommand(
          *p_command, [p_command](const boost::system::error_code&, size_t) {});
    }
  }

  // Execute an handler bound to the serial command (e.g, callback after a
  // service started)
  this->ExecuteAndRemoveCommandHandler(command_serial_received_);

  this->ListenForCommand();
}

//-----------------------------------------------------------------------------
template <typename Demux>
void Admin<Demux>::StartRemoteService(
    const admin::CreateServiceRequest<demux>& create_request,
    const CommandHandler& handler) {
  this->Command(create_request, handler);
}

//-----------------------------------------------------------------------------
template <typename Demux>
void Admin<Demux>::StopRemoteService(
    const admin::StopServiceRequest<demux>& stop_request,
    const CommandHandler& handler) {
  this->Command(stop_request, handler);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template <typename Demux>
void Admin<Demux>::SendKeepAlive(const boost::system::error_code& ec) {
  if (!ec) {
    auto p_command = std::make_shared<AdminCommand>(
        0, reserved_keep_alive_id_, reserved_keep_alive_size_,
        reserved_keep_alive_parameters_);

    auto self = this->SelfFromThis();

    auto do_handler = [self, p_command](
        const boost::system::error_code& ec,
        size_t length) { self->PostKeepAlive(ec, length); };

    this->async_SendCommand(*p_command, do_handler);
  } else {
    this->get_io_service().post(
      boost::bind(&Admin<Demux>::ShutdownServices, SelfFromThis()));
    return;
  }
}

//-----------------------------------------------------------------------------
template <typename Demux>
void Admin<Demux>::PostKeepAlive(const boost::system::error_code& ec,
                                   size_t length) {
  if (!ec) {
    reserved_keep_alive_timer_.expires_from_now(
        boost::posix_time::seconds(keep_alive_interval));  // to define
    reserved_keep_alive_timer_.async_wait(
      boost::bind(&Admin::SendKeepAlive, SelfFromThis(), _1));
  } else {
    this->get_io_service().post(
      boost::bind(&Admin<Demux>::ShutdownServices, SelfFromThis()));
    return;
  }
}

//-----------------------------------------------------------------------------
template <typename Demux>
void Admin<Demux>::ShutdownServices() {
  boost::recursive_mutex::scoped_lock lock(stopping_mutex_);
  if (!stopped_) {
    demux& d = this->get_demux();
    auto p_service_factory =
        ServiceFactoryManager<demux>::GetServiceFactory(&d);
    if (p_service_factory) {
      p_service_factory->Destroy();
    }
    stopped_ = true;
  }
}

//-----------------------------------------------------------------------------
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
