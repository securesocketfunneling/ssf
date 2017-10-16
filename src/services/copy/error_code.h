#ifndef SSF_SERVICES_COPY_ERROR_CODE_H_
#define SSF_SERVICES_COPY_ERROR_CODE_H_

#include <boost/system/error_code.hpp>

namespace ssf {
namespace services {
namespace copy {

enum ErrorCode {
  kSuccess = 0,

  kUnknown = 200,
  kFailure,
  kInterrupted,
  kFilesPartiallyCopied,
  kNoFileCopied,

  kInboundPacketNotSupported,
  kOutboundPacketNotGenerated,

  kInitRequestPacketNotGenerated,
  kInitRequestPacketCorrupted,
  kInitReplyPacketNotGenerated,
  kInitReplyPacketCorrupted,
  kIntegrityCheckRequestPacketNotGenerated,
  kIntegrityCheckRequestPacketCorrupted,
  kIntegrityCheckReplyPacketNotGenerated,
  kIntegrityCheckReplyPacketCorrupted,

  kInputDirectoryNotFound,
  kOutputDirectoryNotFound,
  kOutputFileDirectoryNotFound,

  kInputFileNotAvailable,
  kOutputFileNotAvailable,

  kInputFileReadError,
  kOutputFileWriteError,

  kInputFileDigestNotAvailable,
  kOutputFileDigestNotAvailable,

  kResumeFileTransferNotPermitted,

  kOutputFileCorrupted,

  kCopyInitializationFailed,
  kSenderInputFileListingFailed,

  kCopyRequestAckNotReceived,
  kCopyRequestCorrupted,

  kClientCopyRequestNotSent,

  kFileAcceptorNotBound,
  kFileAcceptorNotListening,
};

namespace detail {
class copy_category : public boost::system::error_category {
 public:
  const char* name() const BOOST_SYSTEM_NOEXCEPT;

  std::string message(int value) const;
};
}  // detail

inline const boost::system::error_category& get_copy_category() {
  static detail::copy_category instance;
  return instance;
}

}  // copy
}  // services
}  // ssf

#endif  // SSF_SERVICES_COPY_ERROR_CODE_H_
