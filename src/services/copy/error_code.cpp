#include "services/copy/error_code.h"

namespace ssf {
namespace services {
namespace copy {

const char* detail::copy_category::name() const BOOST_SYSTEM_NOEXCEPT {
  return "copy";
}

std::string detail::copy_category::message(int value) const {
  switch (value) {
    case kSuccess:
      return "success";
    case kUnknown:
      return "unknown error";
    case kFailure:
      return "failure";
    case kInterrupted:
      return "copy interrupted";
    case kFilesPartiallyCopied:
      return "files partially copied";
    case kNoFileCopied:
      return "no file copied";
    case kInboundPacketNotSupported:
      return "inbound packet not supported";
    case kOutboundPacketNotGenerated:
      return "outbound packet not generated";
    case kInitRequestPacketNotGenerated:
      return "init request packet not generated";
    case kInitRequestPacketCorrupted:
      return "init request packet corrupted";
    case kInitReplyPacketNotGenerated:
      return "init reply packet not generated";
    case kInitReplyPacketCorrupted:
      return "init reply packet corrupted";
    case kIntegrityCheckRequestPacketNotGenerated:
      return "integrity check request packet not generated";
    case kIntegrityCheckRequestPacketCorrupted:
      return "integrity check request packet corrupted";
    case kIntegrityCheckReplyPacketNotGenerated:
      return "intergrity check reply packet not generated";
    case kIntegrityCheckReplyPacketCorrupted:
      return "integrity check reply packet corrupted";
    case kInputDirectoryNotFound:
      return "input directory not found";
    case kOutputDirectoryNotFound:
      return "output directory not found";
    case kOutputFileDirectoryNotFound:
      return "output file directory not found";
    case kInputFileNotAvailable:
      return "input file not available";
    case kOutputFileNotAvailable:
      return "output file not available";
    case kInputFileReadError:
      return "input file read error";
    case kOutputFileWriteError:
      return "output file write error";
    case kInputFileDigestNotAvailable:
      return "input file digest not available";
    case kOutputFileDigestNotAvailable:
      return "output file digest not available";
    case kResumeFileTransferNotPermitted:
      return "resume file transfer not permitted";
    case kOutputFileCorrupted:
      return "output file corrupted";
    case kCopyInitializationFailed:
      return "copy initialization failed";
    case kSenderInputFileListingFailed:
      return "sender input file listing failed";
    case kCopyRequestAckNotReceived:
      return "copy request ack not received";
    case kCopyRequestCorrupted:
      return "copy request corrupted";
    case kClientCopyRequestNotSent:
      return "client copy request not sent";
    case kFileAcceptorNotBound:
      return "file acceptor not bound";
    case kFileAcceptorNotListening:
      return "file acceptor not listening";
    default:
      std::string generic_error("generic copy error ");
      generic_error += std::to_string(value);
      return generic_error;
  };
}

}  // copy
}  // services
}  // ssf