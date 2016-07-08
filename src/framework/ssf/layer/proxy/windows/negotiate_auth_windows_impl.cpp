#include "ssf/layer/proxy/windows/negotiate_auth_windows_impl.h"

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

NegotiateAuthWindowsImpl::NegotiateAuthWindowsImpl(const Proxy& proxy_ctx)
    : NegotiateAuthImpl(proxy_ctx),
      h_cred_(),
      h_sec_ctx_(),
      output_token_(),
      output_token_length_(0),
      service_name_() {}

NegotiateAuthWindowsImpl::~NegotiateAuthWindowsImpl() {}

bool NegotiateAuthWindowsImpl::Init() {
  // determine max output token buffer size
  PSecPkgInfoA sec_package;
  auto status = ::QuerySecurityPackageInfoA("Negotiate", &sec_package);
  if (status != SEC_E_OK) {
    state_ = State::kFailure;
    return false;
  }

  output_token_.resize(sec_package->cbMaxToken);
  ::FreeContextBuffer(sec_package);

  // build spn for proxy (HTTP/proxy_address)
  service_name_ = "HTTP/" + proxy_ctx_.addr;

  return true;
}

bool NegotiateAuthWindowsImpl::ProcessServerToken(const Token& server_token) {
  TimeStamp expiry;
  SecBufferDesc in_sec_buf_desc;
  SecBuffer in_sec_buff;
  SecBufferDesc out_sec_buff_desc;
  SecBuffer out_sec_buff;

  out_sec_buff_desc.ulVersion = SECBUFFER_VERSION;
  out_sec_buff_desc.cBuffers = 1;
  out_sec_buff_desc.pBuffers = &out_sec_buff;
  out_sec_buff.BufferType = SECBUFFER_TOKEN;
  out_sec_buff.pvBuffer = output_token_.data();
  out_sec_buff.cbBuffer = output_token_.size();

  if (server_token.empty()) {
    if (state_ != State::kInit) {
      return false;
    }

    // first response
    bool user_credentials = false;
    // init identity with user credentials if provided
    SEC_WINNT_AUTH_IDENTITY_A identity;

    auto status = ::AcquireCredentialsHandleA(
        NULL, "Negotiate", SECPKG_CRED_OUTBOUND, NULL,
        user_credentials ? &identity : NULL, NULL, NULL, &h_cred_, &expiry);

    if (status != SEC_E_OK) {
      state_ = State::kFailure;
      return false;
    }

    state_ = State::kContinue;
  } else {
    // input token
    in_sec_buf_desc.ulVersion = SECBUFFER_VERSION;
    in_sec_buf_desc.cBuffers = 1;
    in_sec_buf_desc.pBuffers = &in_sec_buff;
    in_sec_buff.BufferType = SECBUFFER_TOKEN;
    in_sec_buff.pvBuffer = const_cast<uint8_t*>(server_token.data());
    in_sec_buff.cbBuffer = server_token.size();
  }

  // update security context
  unsigned long attrs;
  auto status = ::InitializeSecurityContextA(
      &h_cred_, NULL, const_cast<SEC_CHAR*>(service_name_.c_str()),
      ISC_REQ_CONFIDENTIALITY, 0, SECURITY_NATIVE_DREP,
      server_token.empty() ? NULL : &in_sec_buf_desc, 0, &h_sec_ctx_,
      &out_sec_buff_desc, &attrs, &expiry);

  switch (status) {
    case SEC_E_OK:
      state_ = State::kSuccess;
      break;
    case SEC_I_COMPLETE_AND_CONTINUE:
    case SEC_I_COMPLETE_NEEDED: {
      auto complete_status =
          ::CompleteAuthToken(&h_sec_ctx_, &out_sec_buff_desc);
      state_ = complete_status == SEC_E_OK ? State::kContinue : State::kFailure;
      break;
    }
    case SEC_I_CONTINUE_NEEDED:
      state_ = State::kContinue;
      break;
    default:
      state_ = State::kFailure;
  }

  output_token_length_ = out_sec_buff.cbBuffer;

  return state_ != kFailure;
}

NegotiateAuthWindowsImpl::Token NegotiateAuthWindowsImpl::GetAuthToken() {
  if (state_ == kFailure) {
    return {};
  }

  auto begin_it = output_token_.begin();
  auto end_it = begin_it + output_token_length_;

  return Token(begin_it, end_it);
}

}  // detail
}  // proxy
}  // layer
}  // ssf
