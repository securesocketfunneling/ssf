#ifndef SSF_CORE_NETWORK_VIRTUAL_LAYER_POLICIES_BOUNCE_PROTOCOL_POLICY_H_
#define SSF_CORE_NETWORK_VIRTUAL_LAYER_POLICIES_BOUNCE_PROTOCOL_POLICY_H_

#include <cstdint>

#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <list>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>
#include <boost/system/error_code.hpp>
#include <boost/bind.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/buffer.hpp>

#include <boost/log/trivial.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include "common/config/config.h"

#include "common/network/network_policy_traits.h"

#include "common/network/manager.h"
#include "common/network/base_session.h"
#include "common/network/session_forwarder.h"

#include "core/parser/bounce_parser.h"

#include "versions.h"

namespace ssf {

template <typename LinkPolicy, template <class> class LinkAuthenticationPolicy>
class BounceProtocolPolicy
    : public LinkPolicy,
      public LinkAuthenticationPolicy<typename SocketOf<LinkPolicy>::type> {
public:
  typedef typename SocketOf<LinkPolicy>::type socket_type;
  typedef typename SocketPtrOf<LinkPolicy>::type p_socket_type;
  typedef typename AcceptorOf<LinkPolicy>::type acceptor_type;
  typedef typename AcceptorPtrOf<LinkPolicy>::type p_acceptor_type;
  typedef std::map<std::string, std::string> Parameters;

private:
  using BounceParser = ssf::parser::BounceParser;
  typedef std::shared_ptr<uint32_t> p_uint32_t;
  typedef std::shared_ptr<std::vector<uint32_t>> p_vector_uint32_t;
  typedef std::vector<boost::system::error_code> vector_error_code_type;
  typedef std::shared_ptr<boost::asio::streambuf> p_streambuf;

  typedef std::function<void(p_socket_type, vector_error_code_type)>
    callback_type;

  typedef std::map<uint32_t, p_acceptor_type> acceptor_map_type;

  typedef ItemManager<BaseSessionPtr> SessionManager;
  typedef SessionForwarder<socket_type, socket_type>
    SocketSessionForwarder;  

public:
  virtual ~BounceProtocolPolicy() {}

  BounceProtocolPolicy(boost::asio::io_service& io_service,
                       const ssf::Config& ssf_config)
      : LinkPolicy(io_service, ssf_config),
        io_service_(io_service) {}

  void AddRoute(Parameters& parameters, callback_type callback) {
    std::list<std::string> bouncing_nodes = this->GetBouncingNodes(parameters);
    parameters["local"] = "true";

    size_t total_size = bouncing_nodes.size() + 1;
    auto p_ec_values = std::make_shared<std::vector<uint32_t>>(total_size, 0);

    auto handler = [this, parameters, p_ec_values, total_size](
        p_uint32_t p_ec_value, p_socket_type p_socket, callback_type callback) {
      this->GetAllEcs(parameters, p_ec_values, 0, (uint32_t)total_size, p_ec_value, p_socket,
                        callback);
    };

    auto handler_to_do_add_route = [this, handler, callback](
        Parameters parameters, p_socket_type p_socket,
        const boost::system::error_code& ec) {
      if (!ec) {
        BOOST_LOG_TRIVIAL(trace) << "network: do add route";
        this->DoAddRoute(handler, parameters, callback);
      } else {
        BOOST_LOG_TRIVIAL(trace) << "network: handler to add route end";
        this->ProtocolEnd(nullptr, ec, callback);
      }
    };

    this->GetCredentials(parameters, handler_to_do_add_route, nullptr);
  }

  void DeleteRoute(socket_type& socket) {
    BOOST_LOG_TRIVIAL(trace) << "network: delete route";
    this->CloseLink(socket);
  }

  void AcceptNewRoutes(uint16_t port, callback_type callback) {
    auto p_acceptor = std::make_shared<acceptor_type>(io_service_);

    boost::system::error_code ec;
    InitAcceptor(*p_acceptor, port, ec);

    if (!ec) {
      {
        boost::recursive_mutex::scoped_lock lock(acceptors_mutex_);
        acceptors_[port] = p_acceptor;
      }
      // Link policy
      this->AcceptLinks(
          p_acceptor,
          boost::bind(&BounceProtocolPolicy::NewLinkConnectedHandler, this,
                      callback, _1));
    } else {
      auto p_socket = p_socket_type(nullptr);
      this->ProtocolEnd(p_socket, ec, callback);
    }
  }

  void StopAcceptingRoutes() {
    boost::recursive_mutex::scoped_lock lock(acceptors_mutex_);
    for (auto& p_acceptor : acceptors_) {
      boost::system::error_code ec;
      p_acceptor.second->close(ec);
    }
    acceptors_.clear();
  }

 private:
  static void InitAcceptor(acceptor_type& acceptor, uint16_t port,
                            boost::system::error_code& ec) {
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
    boost::asio::socket_base::reuse_address option(true);

    acceptor.open(endpoint.protocol(), ec);
    if (ec) {
      BOOST_LOG_TRIVIAL(error) << "network: error when opening acceptor "
        << ec.message() << " " << ec.value() << std::endl;
      return;
    }

    acceptor.set_option(option, ec);
    if (ec) {
      BOOST_LOG_TRIVIAL(error)
        << "network: error when setting option "
        << ec.message() << " " << ec.value() << std::endl;
      return;
    }

    acceptor.bind(endpoint, ec);
    if (ec) {
      BOOST_LOG_TRIVIAL(error) << "network: error when binding "
        << ec.message() << " " << ec.value() << std::endl;
      return;
    }

    acceptor.listen(100, ec);
    if (ec) {
      BOOST_LOG_TRIVIAL(error) << "network: error when listening "
        << ec.message() << " " << ec.value() << std::endl;
      return;
    }
  }

  static void CloseAcceptor(acceptor_type& acceptor) {
    boost::system::error_code ec;
    acceptor.cancel(ec);
    acceptor.close(ec);
  }

 private:

  template <typename Handler>
  void DoAddRoute(Handler handler, const Parameters& parameters,
                    callback_type callback) {
    BOOST_LOG_TRIVIAL(trace) << "network: establish link";
    this->EstablishLink(
        parameters,
        boost::bind(&BounceProtocolPolicy::LinkEstablishedHandler<Handler>,
                    this, handler, parameters, callback, _1, _2));
  }

  template <typename Handler>
  void LinkEstablishedHandler(Handler handler, const Parameters& parameters,
    callback_type callback, p_socket_type p_socket,
    const boost::system::error_code& ec) {  
      BOOST_LOG_TRIVIAL(trace) << "network: link established handler";
      if (!ec && p_socket) {
        auto p_version = std::make_shared<uint32_t>(0);

        boost::asio::async_read(
            *p_socket, boost::asio::buffer(p_version.get(), sizeof(*p_version)),
            boost::bind(
                &BounceProtocolPolicy::BounceVersionReceivedHandler<Handler>,
                this, handler, parameters, callback, p_version, p_socket, _1,
                _2));
      } else {
        if (p_socket) {
          this->CloseLink(*p_socket);
        }
        this->ProtocolEnd(nullptr, ec, callback);
        return;
      }
  }

  template <typename Handler>
  void BounceVersionReceivedHandler(
      Handler handler, const Parameters& parameters, callback_type callback,
      std::shared_ptr<uint32_t> p_version, p_socket_type p_socket,
      const boost::system::error_code& ec, size_t length) {
    if (!ec) {
      BOOST_LOG_TRIVIAL(trace) << "network: bounce version received: "
        << *p_version;

      if (IsSupportedVersion(*p_version)) {
        auto p_answer = std::make_shared<uint32_t>(1);
        boost::asio::async_write(
            *p_socket, boost::asio::buffer(p_answer.get(), sizeof(*p_answer)),
            boost::bind(
                &BounceProtocolPolicy::BounceAnswerSentHandler<Handler>,
                this, handler, parameters, callback, p_answer, p_socket, _1,
                _2));
      } else {
        BOOST_LOG_TRIVIAL(error) << "network: bounce version NOT supported "
                                 << *p_version;
        boost::system::error_code result_ec(::error::wrong_protocol_type,
                                            ::error::get_ssf_category());
        this->CloseLink(*p_socket);
        this->ProtocolEnd(nullptr, result_ec, callback);
        return;
      }
    } else {
      BOOST_LOG_TRIVIAL(error) << "network: bounce version NOT received "
                               << ec.message();
      this->CloseLink(*p_socket);
      this->ProtocolEnd(nullptr, ec, callback);
      return;
    }
  }

  template <typename Handler>
  void BounceAnswerSentHandler(Handler handler, const Parameters& parameters,
                                  callback_type callback,
                                  std::shared_ptr<uint32_t> p_answer,
                                  p_socket_type p_socket,
                                  const boost::system::error_code& ec,
                                  size_t length) {
    if (!ec) {
      BOOST_LOG_TRIVIAL(info) << "network: bounce answer sent";

      if (*p_answer) {
        BOOST_LOG_TRIVIAL(info) << "network: bounce answer OK";
        std::list<std::string> bouncing_nodes = this->GetBouncingNodes(parameters);

        std::ostringstream ostrs;
        boost::archive::text_oarchive ar(ostrs);

        ar << BOOST_SERIALIZATION_NVP(bouncing_nodes);

        std::string result(ostrs.str());

        auto p_bounce_size = std::make_shared<uint32_t>((uint32_t)result.size());

        boost::asio::async_write(
            *p_socket,
            boost::asio::buffer(p_bounce_size.get(), sizeof(*p_bounce_size)),
            boost::bind(
                &BounceProtocolPolicy::SentBounceSizeHandler<Handler>, this,
                handler, result, bouncing_nodes.size(), p_bounce_size, p_socket,
                callback, _1, _2));
      } else {
        boost::system::error_code result_ec(::error::wrong_protocol_type,
          ::error::get_ssf_category());
        BOOST_LOG_TRIVIAL(error) << "network: bounce answer NOT ok "
                                 << result_ec.message();
        this->CloseLink(*p_socket);
        this->ProtocolEnd(nullptr, result_ec, callback);
        return;
      }
    } else {
      BOOST_LOG_TRIVIAL(error) << "network: could NOT send Bounce answer "
                               << ec.message();
      this->CloseLink(*p_socket);
      this->ProtocolEnd(nullptr, ec, callback);
      return;
    }
  }

  template <typename Handler>
  void SentBounceSizeHandler(Handler handler, std::string result,
                                size_t bounce_number, p_uint32_t p_bounce_size,
                                p_socket_type p_socket, callback_type callback,
                                const boost::system::error_code& ec,
                                size_t length) {
    if (!ec) {
      auto p_buf = std::make_shared<boost::asio::streambuf>();
      std::ostream os(p_buf.get());
      os << result;

      boost::asio::async_write(
          *p_socket, p_buf->data(),
          boost::bind(
              &BounceProtocolPolicy::SentBouncingNodesHandler<Handler>, this,
              handler, bounce_number, p_buf, p_socket, callback, _1, _2));
    } else {
      this->CloseLink(*p_socket);
      ProtocolEnd(p_socket, ec, callback);
    }
  }

  template <typename Handler>
  void SentBouncingNodesHandler(Handler handler, size_t bounce_number,
                                   p_streambuf p_buf, p_socket_type p_socket,
                                   callback_type callback,
                                   const boost::system::error_code& ec,
                                   size_t length) {
    if (!ec) {
      this->ReceiveOneEc(handler, p_socket, callback);
    } else {
      this->CloseLink(*p_socket);
      this->ProtocolEnd(p_socket, ec, callback);
    }
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  template <typename Handler>
  void ReceiveOneEc(Handler handler, p_socket_type p_socket,
                      callback_type callback) {
    auto p_ec_value = std::make_shared<uint32_t>(0);
    BOOST_LOG_TRIVIAL(trace) << "network: bounce protocol receive one ec async read some";
    boost::asio::async_read(
        *p_socket, boost::asio::buffer(p_ec_value.get(), sizeof(*p_ec_value)),
        boost::bind(&BounceProtocolPolicy::ReceivedOneEcHandler<Handler>, this,
                    handler, p_ec_value, p_socket, callback, _1, _2));
  }

  template <typename Handler>
  void ReceivedOneEcHandler(Handler handler, p_uint32_t p_ec_value,
                               p_socket_type p_socket, callback_type callback,
                               const boost::system::error_code& ec,
                               size_t length) {
    if (!ec) {
      handler(p_ec_value, p_socket, callback);
    } else {
      this->CloseLink(*p_socket);
      *p_ec_value = ec.value();
      handler(p_ec_value, p_socket_type(nullptr), callback);
    }
  }

  void GetAllEcs(const Parameters& parameters, p_vector_uint32_t p_ec_values,
                   uint32_t already_done, uint32_t total_size,
                   p_uint32_t p_ec_value, p_socket_type p_socket,
                   callback_type callback) {
    BOOST_LOG_TRIVIAL(trace) << "network: get all ecs done : " << already_done;

    if (already_done < total_size) {
      (*p_ec_values)[already_done] = *p_ec_value;
    }

    if (already_done < total_size - 1) {
      if (!(*p_ec_value)) {

        auto handler = [this, p_ec_values, already_done, total_size, callback](
            const Parameters& parameters, p_socket_type p_socket,
            const boost::system::error_code& ec) {
          if (!ec) {
            this->ReceiveOneEc<
                std::function<void(p_uint32_t, p_socket_type, callback_type)>>(
                boost::bind(&BounceProtocolPolicy::GetAllEcs, this,
                            parameters, p_ec_values, already_done + 1,
                            total_size, _1, _2, _3),
                p_socket, callback);
          } else {
            (*p_ec_values)[already_done] = ec.value();
            this->ProtocolEnd(p_socket, p_ec_values, callback);
          }
        };
        
        Parameters params;
        params["bouncing_nodes"] = parameters.find("bouncing_nodes")->second;
        params["node_id"] = std::to_string(already_done);

        this->SetCredentials(params, handler, p_socket);
      } else {
        this->ProtocolEnd(p_socket, p_ec_values, callback);
      }
    } else {
      this->ProtocolEnd(p_socket, p_ec_values, callback);
    }
  }

  template <typename Handler>
  void SendOneEc(Handler handler, p_uint32_t p_ec_value,
                   p_socket_type p_socket, callback_type callback) {
    boost::asio::async_write(
        *p_socket, boost::asio::buffer(p_ec_value.get(), sizeof(*p_ec_value)),
        boost::bind(&BounceProtocolPolicy::SentOneEcHandler<Handler>, this,
                    p_ec_value, p_socket, handler, callback, _1, _2));
  }

  template <typename Handler>
  void SentOneEcHandler(p_uint32_t p_ec_value, p_socket_type p_socket,
                           Handler handler, callback_type callback,
                           const boost::system::error_code& ec, size_t length) {
    if (!ec) {
      handler(p_socket, callback);
    } else {
      this->CloseLink(*p_socket);
      this->ProtocolEnd(p_socket, ec, callback);
    }
  }

  void NewLinkConnectedHandler(callback_type callback, p_socket_type p_socket) {
    if (!p_socket) {
      boost::system::error_code ec(::error::not_a_socket,
                                   ::error::get_ssf_category());
      this->ProtocolEnd(nullptr, vector_error_code_type(1, ec), callback);
      return;
    }

    BOOST_LOG_TRIVIAL(info) << "network: starting bounce protocol";

    auto p_version = std::make_shared<uint32_t>(GetVersion());

    boost::asio::async_write(
        *p_socket, boost::asio::buffer(p_version.get(), sizeof(*p_version)),
        boost::bind(&BounceProtocolPolicy::BounceVersionSentHandler, this,
                    callback, p_version, p_socket, _1, _2));
  }

  void BounceVersionSentHandler(callback_type callback,
                                   std::shared_ptr<uint32_t> p_version,
                                   p_socket_type p_socket,
                                   const boost::system::error_code& ec,
                                   size_t length) {
    if (!ec) {
      BOOST_LOG_TRIVIAL(info) << "network: bounce request sent";

      auto p_answer = std::make_shared<uint32_t>(0);

      boost::asio::async_read(
          *p_socket, boost::asio::buffer(p_answer.get(), sizeof(*p_answer)),
          boost::bind(&BounceProtocolPolicy::BounceAnwserReceivedHandler,
                      this, callback, p_answer, p_socket, _1, _2));
    } else {
      BOOST_LOG_TRIVIAL(error) << "network: could NOT send the Bounce request "
                               << ec.message();
      this->CloseLink(*p_socket);
      this->ProtocolEnd(nullptr, vector_error_code_type(1, ec), callback);
      return;
    }
  }

  void BounceAnwserReceivedHandler(callback_type callback,
                                      std::shared_ptr<uint32_t> p_answer,
                                      p_socket_type p_socket,
                                      const boost::system::error_code& ec,
                                      size_t length) {
    if (!ec) {
      BOOST_LOG_TRIVIAL(info) << "network: bounce answer received";

      if (*p_answer) {
        BOOST_LOG_TRIVIAL(info) << "network: bounce answer OK";
        auto p_bounce_size = std::make_shared<uint32_t>(0);

        boost::asio::async_read(
            *p_socket,
            boost::asio::buffer(p_bounce_size.get(), sizeof(*p_bounce_size)),
            boost::bind(&BounceProtocolPolicy::ReceivedBounceSizeHandler,
                        this, p_bounce_size, p_socket, callback, _1, _2));
      } else {
        boost::system::error_code result_ec(ssf::error::wrong_protocol_type,
          ssf::error::get_ssf_category());
        BOOST_LOG_TRIVIAL(error) << "network: bounce anwser NOT ok "
          << ec.message();

        this->CloseLink(*p_socket);
        this->ProtocolEnd(nullptr, vector_error_code_type(1, result_ec),
                           callback);
        return;
      }
    } else {
      BOOST_LOG_TRIVIAL(error) << "network: could NOT receive the bounce answer "
                               << ec.message();
      this->CloseLink(*p_socket);
      this->ProtocolEnd(nullptr, vector_error_code_type(1, ec), callback);
      return;
    }
  }

  void ReceivedBounceSizeHandler(p_uint32_t p_bounce_size,
                                    p_socket_type p_socket,
                                    callback_type callback,
                                    const boost::system::error_code& ec,
                                    size_t length) {
    if (!ec) {
      auto p_buffer = std::make_shared<boost::asio::streambuf>();
      boost::asio::streambuf::mutable_buffers_type bufs =
          p_buffer->prepare(*p_bounce_size);

      BOOST_LOG_TRIVIAL(trace) << "network: bounce protocol receive bounce size";
      boost::asio::async_read(
          *p_socket, bufs,
          boost::bind(&BounceProtocolPolicy::ReceivedBouncingNodesHandler,
                      this, p_buffer, p_socket, callback, _1, _2));
    } else {
      this->CloseLink(*p_socket);
      this->ProtocolEnd(p_socket, vector_error_code_type(1, ec), callback);
    }
  }

  void ReceivedBouncingNodesHandler(p_streambuf p_buf,
                                       p_socket_type p_socket,
                                       callback_type callback,
                                       const boost::system::error_code& ec,
                                       size_t length) {
    if (!ec) {
      p_buf->commit(length);
      std::istream is(p_buf.get());
      boost::archive::text_iarchive ar(is);
      std::list<std::string> bouncing_nodes;

      try {
        ar >> BOOST_SERIALIZATION_NVP(bouncing_nodes);

        auto p_ec_value = std::make_shared<uint32_t>(ec.value());

        SendOneEc<std::function<void(p_socket_type, callback_type)>>(
          boost::bind(&BounceProtocolPolicy::EstablishRoute, this,
          bouncing_nodes, _1, _2),
          p_ec_value, p_socket, callback);
        return;
      } catch (const std::exception&) {
        this->CloseLink(*p_socket);
        this->ProtocolEnd(
            p_socket,
            vector_error_code_type(
                1, boost::system::error_code(ssf::error::protocol_error,
                                             ssf::error::get_ssf_category())),
            callback);
        return;
      }
    } else {
      this->CloseLink(*p_socket);
      this->ProtocolEnd(p_socket, vector_error_code_type(1, ec), callback);
      return;
    }
  }

  void EstablishRoute(const std::list<std::string>& bouncing_nodes,
                       p_socket_type p_socket, callback_type callback) {
    if (!bouncing_nodes.size()) {
      this->ProtocolEnd(p_socket, 0, callback);
    } else {
      this->ForwardLink(bouncing_nodes, p_socket);
    }
  }

  //-------------------------------------------------------

  void ForwardLink(std::list<std::string> bouncing_nodes,
                    p_socket_type p_socket_in) {
    std::string remote_endpoint_string = this->PopEndpointString(bouncing_nodes);
    std::string remote_addr = BounceParser::GetRemoteAddress(remote_endpoint_string);
    std::string remote_port = BounceParser::GetRemotePort(remote_endpoint_string);

    if (remote_addr == "" || remote_port == "") {
      this->CloseLink(*p_socket_in);
      return;
    }
    BOOST_LOG_TRIVIAL(trace) << "network: forward link "
      << remote_addr << ":" << remote_port;

    Parameters parameters;
    parameters["remote_addr"] = remote_addr;
    parameters["remote_port"] = remote_port;

    std::ostringstream ostrs;
    boost::archive::text_oarchive ar(ostrs);
    ar << BOOST_SERIALIZATION_NVP(bouncing_nodes);
    std::string bouncers(ostrs.str());

    parameters["bouncing_nodes"] = bouncers;

    auto callback = [this, p_socket_in](p_socket_type p_socket_out,
                                        vector_error_code_type v_ec) {
      if (v_ec[0]) {
        this->CloseLink(*p_socket_in);
      }
    };

    std::function<void(p_socket_type, p_socket_type, callback_type)>
        handler_to_establish_session = [this](p_socket_type p_socket_out,
                                              p_socket_type p_socket_in,
                                              callback_type) {
      if (p_socket_in && p_socket_out) {
        this->EstablishSession(p_socket_in, p_socket_out);
      } else {
        this->CloseLink(*p_socket_in);
      }
    };

    auto handler_to_send_one_ec = [this, p_socket_in,
                                   handler_to_establish_session](
        p_uint32_t p_ec_value, p_socket_type p_socket_out,
        callback_type callback) {
      this->SendOneEc<std::function<void(p_socket_type, callback_type)>>(
          boost::bind(handler_to_establish_session, p_socket_out, _1, _2),
          p_ec_value, p_socket_in, callback);
    };

    auto handler_to_do_add_route = [this, handler_to_send_one_ec,
                                    callback](
        Parameters parameters, p_socket_type p_socket_in,
        const boost::system::error_code& ec) {
      if (!ec) {
        this->DoAddRoute(handler_to_send_one_ec, parameters, callback);
      } else {
        auto p_ec_value = std::make_shared<uint32_t>(ec.value());
        handler_to_send_one_ec(p_ec_value, p_socket_type(nullptr), callback);
        //this->ProtocolEnd(p_socket_in, ec, callback);
      }
    };

    this->GetCredentials(parameters, handler_to_do_add_route, p_socket_in);
  }

  void EstablishSession(p_socket_type p_socket_in,
                        p_socket_type p_socket_out) {
    boost::system::error_code ec;

    // ! Can't std::move ssl stream ! //
    auto p_session = SocketSessionForwarder::create(
        &manager_, std::move(*p_socket_in), std::move(*p_socket_out));
    manager_.start(p_session, ec);
  }

  //-------------------------------------------------------
  void ProtocolEnd(p_socket_type p_socket, const boost::system::error_code& ec,
                    callback_type callback) {
    this->ProtocolEnd(p_socket, vector_error_code_type(1, ec), callback);
  }

  void ProtocolEnd(p_socket_type p_socket, uint32_t ec_value,
                    callback_type callback) {
    boost::system::error_code ec(ec_value, boost::system::system_category());

    this->ProtocolEnd(p_socket, ec, callback);
  }

  void ProtocolEnd(p_socket_type p_socket, p_vector_uint32_t p_status,
                    callback_type callback) {
    vector_error_code_type v_ec;
    for (size_t i = 0; i < p_status->size(); ++i) {
      v_ec.push_back(boost::system::error_code(
          (*p_status)[i], boost::system::system_category()));
    }

    this->ProtocolEnd(p_socket, v_ec, callback);
  }

  void ProtocolEnd(p_socket_type p_socket, vector_error_code_type v_ec,
                    callback_type callback) {
    io_service_.post(boost::bind(callback, p_socket, v_ec));
  }

  //-------------------------------------------------------

  uint32_t GetVersion() {
    uint32_t version = versions::major;
    version = version << 8;

    version |= versions::minor;
    version = version << 8;

    version |= versions::bounce;
    version = version << 8;

    version |= uint8_t(boost::archive::BOOST_ARCHIVE_VERSION());

    return version;
  }

  bool IsSupportedVersion(uint32_t input_version) {
    boost::archive::library_version_type serialization(input_version &
      0x000000FF);
    input_version = input_version >> 8;

    uint8_t bounce = (input_version & 0x000000FF);
    input_version = input_version >> 8;

    uint8_t minor = (input_version & 0x000000FF);
    input_version = input_version >> 8;

    uint8_t major = (input_version & 0x000000FF);

    return (major == versions::major) && (minor == versions::minor) &&
           (bounce == versions::bounce) &&
           (serialization == boost::archive::BOOST_ARCHIVE_VERSION());
  }

private:
  boost::asio::io_service& io_service_;

  boost::recursive_mutex acceptors_mutex_;
  acceptor_map_type acceptors_;

  SessionManager manager_;
};

}  // ssf

#endif  // SSF_CORE_NETWORK_VIRTUAL_LAYER_POLICIES_BOUNCE_PROTOCOL_POLICY_H_