#ifndef SSF_SERVICES_PROCESS_SESSION_IPP_
#define SSF_SERVICES_PROCESS_SESSION_IPP_


#include <ssf/log/log.h>

namespace ssf {
namespace services {
namespace process {

template <typename Demux>
Session<Demux>::Session(SessionManager *p_session_manager, fiber client)
    : ssf::BaseSession(),
      io_service_(client.get_io_service()),
      p_session_manager_(p_session_manager),
      client_(std::move(client)) {}

template <typename Demux>
void Session<Demux>::start(boost::system::error_code& ec) {
  SSF_LOG(kLogInfo) << "session[process]: start";

}

template <typename Demux>
void Session<Demux>::stop(boost::system::error_code& ec) {
  client_.close();
  if (ec) {
    SSF_LOG(kLogError) << "session[process]: stop error " << ec.message();
  }
}

}  // process
}  // services
}  // ssf

#endif  // SSF_SERVICES_PROCESS_SESSION_IPP_