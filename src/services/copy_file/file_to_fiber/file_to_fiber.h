#ifndef SSF_SERVICES_COPY_FILE_FILE_TO_FIBER_FILE_TO_FIBER_H_
#define SSF_SERVICES_COPY_FILE_FILE_TO_FIBER_FILE_TO_FIBER_H_

#ifdef WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include <cstdint>

#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <string>

#include <boost/algorithm/string/replace.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

#include <ssf/log/log.h>

#include "core/factories/service_factory.h"
#include "services/admin/requests/create_service_request.h"
#include "services/copy_file/file_to_fiber/istream_to_fiber_session.h"

#include "services/copy_file/config.h"
#include "services/copy_file/fiber_to_file/fiber_to_file.h"
#include "services/copy_file/filename_buffer.h"
#include "services/copy_file/filesystem/filesystem.h"

namespace ssf {
namespace services {
namespace copy_file {
namespace file_to_fiber {

template <class Demux>
class FileToFiber : public BaseService<Demux> {
 private:
  using FileToFiberPtr = std::shared_ptr<FileToFiber>;
  using SessionManager = ItemManager<BaseSessionPtr>;
  using Parameters = typename ssf::BaseService<Demux>::Parameters;
  using fiber_port = typename Demux::local_port_type;
  using demux = typename ssf::BaseService<Demux>::demux;
  using fiber = typename ssf::BaseService<Demux>::fiber;
  using FiberPtr = std::shared_ptr<fiber>;
  using endpoint = typename ssf::BaseService<Demux>::endpoint;
  using fiber_acceptor = typename ssf::BaseService<Demux>::fiber_acceptor;
  using FilesList = std::list<std::string>;
  using FilesListPtr = std::shared_ptr<FilesList>;
  using InputOutputFilesMap = std::map<std::string, std::string>;
  using StopSessionHandler = std::function<void(const std::string&)>;
  using Filesystem = ssf::services::copy_file::filesystem::Filesystem;
  using FilenameBuffer = ssf::services::copy_file::FilenameBuffer;

 public:
  enum { kFactoryId = 8, kServicePort = 41 + ((1 << 17) + 1) };

 public:
  // Factory method to create the service
  static FileToFiberPtr Create(boost::asio::io_service& io_service,
                               demux& fiber_demux, Parameters parameters) {
    if (parameters.count("accept") != 0) {
      return FileToFiberPtr(new FileToFiber(io_service, fiber_demux));
    }

    if (parameters.count("from_stdin") != 0 &&
        parameters.count("input_pattern") != 0 &&
        parameters.count("output_pattern") != 0) {
      return FileToFiberPtr(new FileToFiber(
          io_service, fiber_demux, parameters["from_stdin"] == "true",
          parameters["input_pattern"], parameters["output_pattern"]));
    }

    return nullptr;
  }

  static void RegisterToServiceFactory(
      std::shared_ptr<ServiceFactory<demux>> p_factory, const Config& config) {
    if (!config.enabled()) {
      // service factory is not enabled
      return;
    }

    p_factory->RegisterServiceCreator(kFactoryId, &FileToFiber::Create);
  }

  static ssf::services::admin::CreateServiceRequest<demux> GetCreateRequest(
      bool server, bool from_stdin, const std::string& input_pattern,
      const std::string& output_pattern) {
    ssf::services::admin::CreateServiceRequest<demux> create(kFactoryId);
    if (server) {
      create.add_parameter("accept", "true");
    } else {
      create.add_parameter("from_stdin", from_stdin ? "true" : "false");
      create.add_parameter("input_pattern", input_pattern);
      create.add_parameter("output_pattern", output_pattern);
    }

    return create;
  }

 public:
  // Start service
  void start(boost::system::error_code& ec) override {
// set stdin in binary mode
#ifdef WIN32
    if (_setmode(_fileno(stdin), _O_BINARY) == -1) {
      return;
    }
#endif

    if (accept_) {
      endpoint ep(this->get_demux(), kServicePort);
      SSF_LOG(kLogInfo)
          << "microservice[file to fiber]: start accept on fiber port "
          << kServicePort;
      fiber_acceptor_.bind(ep, ec);
      fiber_acceptor_.listen(boost::asio::socket_base::max_connections, ec);
      if (ec) {
        return;
      }
      StartAccept();
    } else {
      StartTransfer();
    }
  }

  // Stop service
  void stop(boost::system::error_code& ec) override {
    SSF_LOG(kLogInfo) << "microservice[file to fiber]: stopping";
    manager_.stop_all();
    boost::system::error_code close_ec;
    fiber_acceptor_.close(close_ec);

// reset stdin in text mode
#ifdef WIN32
    if (_setmode(_fileno(stdin), _O_TEXT) == -1) {
      return;
    }
#endif
  }

  uint32_t service_type_id() override { return kFactoryId; }

 private:
  FileToFiber(boost::asio::io_service& io_service, demux& fiber_demux)
      : BaseService<Demux>(io_service, fiber_demux),
        accept_(true),
        from_stdin_(false),
        input_pattern_(""),
        output_pattern_(""),
        fiber_acceptor_(io_service) {}

  FileToFiber(boost::asio::io_service& io_service, demux& fiber_demux,
              bool from_stdin, const std::string& input_pattern,
              const std::string& output_pattern)
      : BaseService<Demux>(io_service, fiber_demux),
        accept_(false),
        from_stdin_(from_stdin),
        input_pattern_(input_pattern),
        output_pattern_(output_pattern),
        fiber_acceptor_(io_service) {}

  // Start accepting new fiber (remote files to local)
  // Input and output patterns will be given through the fiber
  void StartAccept() {
    if (fiber_acceptor_.is_open()) {
      auto p_fiber =
          std::make_shared<fiber>(this->get_demux().get_io_service());
      fiber_acceptor_.async_accept(
          *p_fiber,
          std::bind(&FileToFiber::AcceptFiberHandler, this->SelfFromThis(),
                    std::placeholders::_1, p_fiber));
    }
  }

  void AcceptFiberHandler(const boost::system::error_code& ec,
                          FiberPtr p_fiber) {
    if (!ec) {
      StartAccept();
      ConfigureSessions(p_fiber, boost::system::error_code(), 0);
    }
  }

#include <boost/asio/yield.hpp>
  // Read input and output pattern from fiber
  // Connect sessions to remote fiber to file service
  void ConfigureSessions(FiberPtr p_fiber, const boost::system::error_code& ec,
                         std::size_t length) {
    if (ec) {
      boost::system::error_code close_ec;
      p_fiber->close(close_ec);
      return;
    }
    reenter(coroutine_) {
      // read input pattern size from request
      yield boost::asio::async_read(
          *p_fiber, input_request_.GetFilenameSizeMutBuffers(),
          std::bind(&FileToFiber::ConfigureSessions, this->SelfFromThis(),
                    p_fiber, std::placeholders::_1, std::placeholders::_2));

      // read input pattern from request
      yield boost::asio::async_read(
          *p_fiber, input_request_.GetFilenameMutBuffers(),
          std::bind(&FileToFiber::ConfigureSessions, this->SelfFromThis(),
                    p_fiber, std::placeholders::_1, std::placeholders::_2));

      // read output pattern size from request
      yield boost::asio::async_read(
          *p_fiber, output_request_.GetFilenameSizeMutBuffers(),
          std::bind(&FileToFiber::ConfigureSessions, this->SelfFromThis(),
                    p_fiber, std::placeholders::_1, std::placeholders::_2));

      // read output pattern from request
      yield boost::asio::async_read(
          *p_fiber, output_request_.GetFilenameMutBuffers(),
          std::bind(&FileToFiber::ConfigureSessions, this->SelfFromThis(),
                    p_fiber, std::placeholders::_1, std::placeholders::_2));

      ProcessInputOutputPattern(input_request_.GetFilename(),
                                output_request_.GetFilename());
    }
  }
#include <boost/asio/unyield.hpp>

  // Start transfer data
  void StartTransfer() {
    if (!from_stdin_) {
      ProcessInputOutputPattern(input_pattern_, output_pattern_);
    } else {
      auto p_output_files_list = std::make_shared<FilesList>();
      p_output_files_list->push_back(output_pattern_);
      auto finish_callback =
          std::bind(&FileToFiber::FinishSentSession, this->SelfFromThis(),
                    std::placeholders::_1, p_output_files_list);
      AsyncConnectFileFiberSession(true, "", output_pattern_, finish_callback);
    }
  }

  // Get the file list and generate one transfer session per input file
  void ProcessInputOutputPattern(const std::string& input_pattern,
                                 const std::string& output_pattern) {
    FilesList input_files_list;
    auto p_output_files_list = std::make_shared<FilesList>();

    InputOutputFilesMap input_output_files;
    ParseInputOutputPatterns(input_pattern, output_pattern,
                             &input_output_files);

    for (auto& input_output_file_pair : input_output_files) {
      p_output_files_list->push_back(input_output_file_pair.second);
    }

    // If no file, callback finish handler
    if (input_output_files.empty()) {
      this->get_io_service().post(std::bind(&FileToFiber::FinishSentSession,
                                              this->SelfFromThis(), "",
                                              p_output_files_list));
      return;
    };

    auto finish_callback =
        std::bind(&FileToFiber::FinishSentSession, this->SelfFromThis(),
                  std::placeholders::_1, p_output_files_list);

    // Start one session per input file
    for (auto& input_output_file_pair : input_output_files) {
      AsyncConnectFileFiberSession(false, input_output_file_pair.first,
                                   input_output_file_pair.second,
                                   finish_callback);
    }
  }

  // Connect a new fiber to the fiber to file service and start an input stream
  // to fiber session
  void AsyncConnectFileFiberSession(bool from_stdin,
                                    const std::string& input_file,
                                    const std::string& output_file,
                                    StopSessionHandler stop_handler) {
    FiberPtr p_fiber =
        std::make_shared<fiber>(this->get_demux().get_io_service());

    SSF_LOG(kLogDebug)
        << "microservice[file to fiber]: connect to remote fiber acceptor port "
        << ssf::services::copy_file::fiber_to_file::FiberToFile<
               Demux>::kServicePort;

    endpoint ep(this->get_demux(),
                ssf::services::copy_file::fiber_to_file::FiberToFile<
                    Demux>::kServicePort);

    p_fiber->async_connect(
        ep, std::bind(&FileToFiber::StartDataForwarderSessionHandler,
                      this->SelfFromThis(), std::placeholders::_1, p_fiber,
                      from_stdin, input_file, output_file, stop_handler));
  }

  // Start the data forward session
  void StartDataForwarderSessionHandler(const boost::system::error_code& ec,
                                        FiberPtr p_fiber, bool from_stdin,
                                        const std::string& input_file,
                                        const std::string& output_file,
                                        StopSessionHandler stop_handler) {
    if (ec) {
      boost::system::error_code close_ec;
      p_fiber->close(close_ec);
      return;
    }

    if (from_stdin) {
      SSF_LOG(kLogInfo)
          << "microservice[file to fiber]: start forward data from stdin to "
          << output_file;
    } else {
      SSF_LOG(kLogInfo)
          << "microservice[file to fiber]: start forward data from "
          << input_file << " to " << output_file;
    }

    if (!from_stdin) {
      auto p_session = IstreamToFiberSession<std::ifstream, fiber>::Create(
          &manager_, input_file, std::move(*p_fiber), output_file,
          stop_handler);
      boost::system::error_code start_ec;
      manager_.start(p_session, start_ec);
    } else {
      auto p_session = IstreamToFiberSession<std::ifstream, fiber>::Create(
          &manager_, std::move(*p_fiber), output_file, stop_handler);
      boost::system::error_code start_ec;
      manager_.start(p_session, start_ec);
    }
  }

  // This callback remove the output file from the file list and closes the
  // demux service once the file list is empty
  void FinishSentSession(const std::string output_file,
                         FilesListPtr p_files_list) {
    std::unique_lock<std::mutex> lock_files_set(files_set_mutex_);
    p_files_list->remove(output_file);

    if (p_files_list->empty()) {
      auto& demux = this->get_demux();
      auto close_demux_callback = [&demux]() { demux.close(); };
      this->get_demux().get_io_service().post(close_demux_callback);
    }
  }

  // Extract input filename from input pattern
  // Generate output filename from input file and output_pattern
  void ParseInputOutputPatterns(const std::string& input_pattern,
                                const std::string& output_pattern,
                                InputOutputFilesMap* p_input_output_files) {
    auto input_files = filesystem::Filesystem::Glob(input_pattern);
    for (auto& input_file : input_files) {
      (*p_input_output_files)[input_file] =
          output_pattern + Filesystem::GetFilename(input_file);
    }
  }

  template <typename Handler, typename This>
  auto Then(Handler handler, This me)
      -> decltype(std::bind(handler, me->SelfFromThis(),
                            std::placeholders::_1)) {
    return std::bind(handler, me->SelfFromThis(), std::placeholders::_1);
  }

  std::shared_ptr<FileToFiber> SelfFromThis() {
    return std::static_pointer_cast<FileToFiber>(this->shared_from_this());
  }

 private:
  bool accept_;
  bool from_stdin_;
  std::string input_pattern_;
  std::string output_pattern_;
  SessionManager manager_;
  fiber_acceptor fiber_acceptor_;
  std::mutex files_set_mutex_;
  // Coroutine variables
  boost::asio::coroutine coroutine_;
  FilenameBuffer input_request_;
  FilenameBuffer output_request_;
};

}  // file_to_fiber
}  // copy_file
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_FILE_FILE_TO_FIBER_FILE_TO_FIBER_H_
