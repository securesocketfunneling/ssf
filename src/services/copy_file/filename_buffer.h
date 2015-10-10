#ifndef SSF_SERVICES_COPY_FILE_FILENAME_BUFFER_H_
#define SSF_SERVICES_COPY_FILE_FILENAME_BUFFER_H_

#include <cstdint>

#include <array>
#include <string>
#include <vector>

#include <boost/asio/buffer.hpp>

namespace ssf {
namespace services {
namespace copy_file {

class FilenameBuffer {
 public:
  FilenameBuffer();
  FilenameBuffer(const std::string& filename);

  std::string GetFilename() const;

  boost::asio::const_buffers_1 GetFilenameSizeConstBuffers() const;
  boost::asio::mutable_buffers_1 GetFilenameSizeMutBuffers();

  boost::asio::const_buffers_1 GetFilenameConstBuffers() const;
  boost::asio::mutable_buffers_1 GetFilenameMutBuffers();

 private:
  uint32_t filename_size_;
  std::vector<char> filename_;
};

}  // copy_file
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_FILE_FILENAME_BUFFER_H_
