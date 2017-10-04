#include <algorithm>

#include <boost/bind.hpp>

#include "ssf/layer/proxy/http_response_builder.h"

#define HTTP_PARSER_C_CALLBACK_IMPL(method)                                  \
  int HttpResponseBuilder::HTTP_PARSER_C_CALLBACK_NAME(method)(http_parser * \
                                                               p_parser) {   \
    HttpResponseBuilder* p_response_builder =                                \
        static_cast<HttpResponseBuilder*>(p_parser->data);                   \
    return p_response_builder->method(p_parser);                             \
  \
}

#define HTTP_PARSER_C_DATA_CALLBACK_IMPL(method)                \
  int HttpResponseBuilder::HTTP_PARSER_C_CALLBACK_NAME(method)( \
      http_parser * p_parser, const char* data, size_t len) {   \
    HttpResponseBuilder* p_response_builder =                   \
        static_cast<HttpResponseBuilder*>(p_parser->data);      \
    return p_response_builder->method(p_parser, data, len);     \
  }

namespace ssf {
namespace layer {
namespace proxy {

HttpResponseBuilder::HttpResponseBuilder()
    : response_(),
      done_(false),
      processing_header_name_(true),
      current_header_name_(),
      current_header_value_(),
      ss_body_() {
  http_parser_init(&parser_, HTTP_RESPONSE);
  parser_.data = this;

  http_parser_settings_init(&parser_settings_);
  parser_settings_.on_message_begin =
      HTTP_PARSER_C_CALLBACK_NAME(OnMessageBegin);
  parser_settings_.on_status = HTTP_PARSER_C_CALLBACK_NAME(OnStatus);
  parser_settings_.on_header_field = HTTP_PARSER_C_CALLBACK_NAME(OnHeaderName);
  parser_settings_.on_header_value = HTTP_PARSER_C_CALLBACK_NAME(OnHeaderValue);
  parser_settings_.on_headers_complete =
      HTTP_PARSER_C_CALLBACK_NAME(OnHeadersComplete);
  parser_settings_.on_body = HTTP_PARSER_C_CALLBACK_NAME(OnBody);
  parser_settings_.on_message_complete =
      HTTP_PARSER_C_CALLBACK_NAME(OnMessageComplete);
}

const HttpResponse* HttpResponseBuilder::Get() const {
  return &response_;
}

void HttpResponseBuilder::Reset() {
  parser_.data = this;
  done_ = false;
  processing_header_name_ = true;
  current_header_name_.clear();
  current_header_value_.clear();
  ss_body_.clear();

  http_parser_init(&parser_, HTTP_RESPONSE);

  response_.Reset();
}

HttpResponseBuilder::ParserStatus HttpResponseBuilder::ProcessInput(
    const char* p_data, std::size_t size) {
  std::size_t parsed_size =
      http_parser_execute(&parser_, &parser_settings_, p_data, size);

  if (parser_.upgrade) {
    // not implemented
    return kParserError;
  }

  if (parsed_size != size) {
    return kParserError;
  }

  return kParserOk;
}

HttpResponseBuilder::ParserStatus HttpResponseBuilder::OnMessageBegin(
    http_parser* p_parser) {
  return kParserOk;
}

HttpResponseBuilder::ParserStatus HttpResponseBuilder::OnStatus(
    http_parser* p_parser, const char* at, size_t length) {
  response_.set_status_code(p_parser->status_code);
  return kParserOk;
}

HttpResponseBuilder::ParserStatus HttpResponseBuilder::OnHeaderName(
    http_parser* p_parser, const char* at, size_t length) {
  if (!processing_header_name_) {
    response_.AddHeader(current_header_name_, current_header_value_);
    current_header_name_.clear();
    current_header_value_.clear();
    processing_header_name_ = true;
  }
  current_header_name_.append(std::string(at, length));
  return kParserOk;
}

HttpResponseBuilder::ParserStatus HttpResponseBuilder::OnHeaderValue(
    http_parser* p_parser, const char* at, size_t length) {
  if (processing_header_name_) {
    std::transform(current_header_name_.begin(), current_header_name_.end(),
                   current_header_name_.begin(), ::tolower);
    processing_header_name_ = false;
  }
  current_header_value_.append(std::string(at, length));
  return kParserOk;
}

HttpResponseBuilder::ParserStatus HttpResponseBuilder::OnHeadersComplete(
    http_parser* p_parser) {
  if (!processing_header_name_) {
    response_.AddHeader(current_header_name_, current_header_value_);
    current_header_name_.clear();
    current_header_value_.clear();
    processing_header_name_ = true;
  }

  if (response_.GetHeaderValues("content-length").empty()) {
    return kParserNoBody;
  }

  return kParserOk;
}

HttpResponseBuilder::ParserStatus HttpResponseBuilder::OnBody(
    http_parser* p_parser, const char* at, size_t length) {
  ss_body_.write(at, length);

  return kParserOk;
}

HttpResponseBuilder::ParserStatus HttpResponseBuilder::OnMessageComplete(
    http_parser* p_parser) {
  response_.set_body(ss_body_.str());

  ss_body_.clear();
  done_ = true;

  return kParserOk;
}

HTTP_PARSER_C_CALLBACK_IMPL(OnMessageBegin)
HTTP_PARSER_C_DATA_CALLBACK_IMPL(OnStatus)
HTTP_PARSER_C_DATA_CALLBACK_IMPL(OnHeaderName)
HTTP_PARSER_C_DATA_CALLBACK_IMPL(OnHeaderValue)
HTTP_PARSER_C_CALLBACK_IMPL(OnHeadersComplete)
HTTP_PARSER_C_DATA_CALLBACK_IMPL(OnBody)
HTTP_PARSER_C_CALLBACK_IMPL(OnMessageComplete)

}  // proxy
}  // layer
}  // ssf
