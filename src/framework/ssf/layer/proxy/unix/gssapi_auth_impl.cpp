#include <dlfcn.h>

#include <list>

#include "ssf/log/log.h"
#include "ssf/layer/proxy/unix/gssapi_auth_impl.h"

static gss_OID_desc GSS_C_NT_HOSTBASED_SERVICE_VAL{
    10, const_cast<char*>("\x2a\x86\x48\x86\xf7\x12\x01\x02\x01\x04")};
static gss_OID_desc GSS_C_NT_HOSTBASED_SERVICE_X_VAL{
    6, const_cast<char*>("\x2b\x06\x01\x05\x06\x02")};
static gss_OID_desc GSS_SPNEGO_MECH_OID_VAL{
    6, const_cast<char*>("\x2b\x06\x01\x05\x05\x02")};

#undef GSS_C_NT_HOST_BASED_SERVICE
#undef GSS_C_NT_HOST_BASED_SERVICE_X

gss_OID GSS_C_NT_HOSTBASED_SERVICE = &GSS_C_NT_HOSTBASED_SERVICE_VAL;
gss_OID GSS_C_NT_HOSTBASED_SERVICE_X = &GSS_C_NT_HOSTBASED_SERVICE_X_VAL;
gss_OID GSS_SPNEGO_MECH_OID = &GSS_SPNEGO_MECH_OID_VAL;

#define GSS_DL_SYM(lib, func) \
  fct_gss_##func##_t func = reinterpret_cast<fct_gss_##func##_t>( \
      dlsym(lib, "gss_" #func)); \
  if (func == NULL) { \
    state_ = kFailure; \
    return false; \
  }

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

GSSAPIAuthImpl::GSSAPIAuthImpl(const Proxy& proxy_ctx)
    : PlatformAuthImpl(proxy_ctx),
      h_gss_api_(NULL),
      h_sec_ctx_(GSS_C_NO_CONTEXT),
      server_name_(GSS_C_NO_NAME),
      auth_token_(),
      fct_gss_init_sec_context_(nullptr),
      fct_gss_import_name_(nullptr),
      fct_gss_release_buffer_(nullptr),
      fct_gss_delete_sec_context_(nullptr),
      fct_gss_release_name_(nullptr) {}

GSSAPIAuthImpl::~GSSAPIAuthImpl() {
  OM_uint32 minor_status;

  if (server_name_ != GSS_C_NO_NAME) {
    fct_gss_release_name_(&minor_status, &server_name_);
  }

  if (h_sec_ctx_ != GSS_C_NO_CONTEXT) {
    fct_gss_delete_sec_context_(&minor_status, &h_sec_ctx_, GSS_C_NO_BUFFER);
  }

  fct_gss_init_sec_context_ = nullptr;
  fct_gss_import_name_ = nullptr;
  fct_gss_release_buffer_ = nullptr;
  fct_gss_delete_sec_context_ = nullptr;
  fct_gss_release_name_ = nullptr;

  if (h_gss_api_ != NULL) {
    dlclose(h_gss_api_);
    h_gss_api_ = NULL;
  }
}

bool GSSAPIAuthImpl::Init() {
  if (!InitLibrary()) {
    SSF_LOG(kLogDebug) << "network[proxy]: gssapi: "
                       << "could not init gssapi library";
    state_ = kFailure;
    return false;
  }

  std::string service_name = "HTTP@" + proxy_ctx_.host;
  OM_uint32 minor_status;
  gss_buffer_desc spn_name;
  spn_name.length = service_name.size();
  spn_name.value = const_cast<char*>(service_name.c_str());
  server_name_ = GSS_C_NO_NAME;
  if (GSS_S_COMPLETE != fct_gss_import_name_(&minor_status, &spn_name,
                                             GSS_C_NT_HOSTBASED_SERVICE,
                                             &server_name_)) {
    SSF_LOG(kLogDebug) << "network[proxy]: gssapi: "
                       << "could not generate gssapi server name";
    state_ = kFailure;
    return false;
  }

  return true;
}

bool GSSAPIAuthImpl::ProcessServerToken(const Token& server_token) {
  if (state_ == kFailure) {
    return false;
  }

  OM_uint32 minor_status;
  OM_uint32 req_flags = 0;

  gss_buffer_desc output_token;

  gss_buffer_desc input_token;
  input_token.length = server_token.size();
  input_token.value = const_cast<uint8_t*>(server_token.data());

  if (state_ == State::kInit && server_token.empty()) {
    // initialize context
    state_ = State::kContinue;
  }

  OM_uint32 maj_status = fct_gss_init_sec_context_(
      &minor_status, GSS_C_NO_CREDENTIAL, &h_sec_ctx_, server_name_,
      GSS_SPNEGO_MECH_OID, req_flags, GSS_C_INDEFINITE,
      GSS_C_NO_CHANNEL_BINDINGS, &input_token, nullptr, &output_token, 0,
      nullptr);

  if (GSS_ERROR(maj_status)) {
    LogError(maj_status);
    state_ = kFailure;
    return false;
  }

  auth_token_.resize(output_token.length);
  std::copy(static_cast<uint8_t*>(output_token.value),
            static_cast<uint8_t*>(output_token.value) + output_token.length,
            auth_token_.begin());

  fct_gss_release_buffer_(&minor_status, &output_token);

  return true;
}

GSSAPIAuthImpl::Token GSSAPIAuthImpl::GetAuthToken() {
  if (state_ == kFailure) {
    return {};
  }

  return auth_token_;
}

bool GSSAPIAuthImpl::InitLibrary() {
  std::list<std::string> lib_names;
#if defined(__APPLE__)
  lib_names.emplace_back(
      "/System/Library/Frameworks/Kerberos.framework/Kerberos");
#else
  lib_names.emplace_back("libgssapi.so");
  lib_names.emplace_back("libgssapi_krb5.so.2");
  lib_names.emplace_back("libgssapi_krb5.so.4");
  lib_names.emplace_back("libgssapi.so.2");
  lib_names.emplace_back("libgssapi.so.1");
#endif
  for (const auto& lib_name : lib_names) {
    h_gss_api_ = dlopen(lib_name.c_str(), RTLD_LAZY);
    if (h_gss_api_ != NULL) {
      break;
    }
  }

  if (h_gss_api_ == NULL) {
    state_ = kFailure;
    return false;
  }

  GSS_DL_SYM(h_gss_api_, init_sec_context);
  GSS_DL_SYM(h_gss_api_, import_name);
  GSS_DL_SYM(h_gss_api_, release_buffer);
  GSS_DL_SYM(h_gss_api_, delete_sec_context);
  GSS_DL_SYM(h_gss_api_, release_name);
  
  fct_gss_init_sec_context_ = init_sec_context;
  fct_gss_import_name_ = import_name;
  fct_gss_release_buffer_ = release_buffer;
  fct_gss_delete_sec_context_ = delete_sec_context;
  fct_gss_release_name_ = release_name;

  return true;
}

void GSSAPIAuthImpl::LogError(OM_uint32 major_status) {
  if (major_status == GSS_S_COMPLETE || major_status == GSS_S_CONTINUE_NEEDED) {
    return;
  }

  std::string error_msg;

  if (GSS_CALLING_ERROR(major_status)) {
    SSF_LOG(kLogDebug) << "network[proxy]: gssapi: calling error";
    return;
  }

  auto routine_status = GSS_ROUTINE_ERROR(major_status);
  switch (routine_status) {
    case GSS_S_DEFECTIVE_TOKEN:
      error_msg = "defective token";
      break;
    case GSS_S_DEFECTIVE_CREDENTIAL:
      error_msg = "defective credential";
      break;
    case GSS_S_BAD_SIG:
      error_msg = "bad sig";
      break;
    case GSS_S_NO_CRED:
      error_msg = "no cred";
      break;
    case GSS_S_CREDENTIALS_EXPIRED:
      error_msg = "credentials expired";
      break;
    case GSS_S_BAD_BINDINGS:
      error_msg = "bad bindings";
      break;
    case GSS_S_NO_CONTEXT:
      error_msg = "no context";
      break;
    case GSS_S_BAD_NAMETYPE:
      error_msg = "bad nametype";
      break;
    case GSS_S_BAD_NAME:
      error_msg = "bad name";
      break;
    case GSS_S_BAD_MECH:
      error_msg = "bad mech";
      break;
    case GSS_S_FAILURE:
      error_msg = "failure";
      break;
    default:
      error_msg = "unknown error";
      break;
  }
  SSF_LOG(kLogDebug) << "network[proxy]: gssapi: " << error_msg;
}

}  // detail
}  // proxy
}  // layer
}  // ssf
