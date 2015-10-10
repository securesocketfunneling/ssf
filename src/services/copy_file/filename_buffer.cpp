#include "services/copy_file/filename_buffer.h"

namespace ssf {
namespace services {
namespace copy_file {

FilenameBuffer::FilenameBuffer() : filename_size_(0), filename_() {}

FilenameBuffer::FilenameBuffer(const std::string& filename)
    : filename_size_(filename.length()),
      filename_(filename.cbegin(), filename.cend()) {}

std::string FilenameBuffer::GetFilename() const {
  return std::string(filename_.cbegin(), filename_.cend());
}

boost::asio::const_buffers_1 FilenameBuffer::GetFilenameSizeConstBuffers()
    const {
  return boost::asio::const_buffers_1(
      boost::asio::buffer(&filename_size_, sizeof(filename_size_)));
}

boost::asio::mutable_buffers_1 FilenameBuffer::GetFilenameSizeMutBuffers() {
  return boost::asio::buffer(&filename_size_, sizeof(filename_size_));
}

boost::asio::const_buffers_1 FilenameBuffer::GetFilenameConstBuffers() const {
  return boost::asio::const_buffers_1(boost::asio::buffer(filename_));
}

boost::asio::mutable_buffers_1 FilenameBuffer::GetFilenameMutBuffers() {
  filename_.resize(filename_size_);
  return boost::asio::buffer(filename_);
}

}  // copy_file
}  // services
}  // ssf