#ifndef SSF_LAYER_PROXY_HTTP_RESPONSE_BUILDER_H_
#define SSF_LAYER_PROXY_HTTP_RESPONSE_BUILDER_H_

#include <map>
#include <string>
#include <sstream>

#include <http-parser/http_parser.h>

#include "ssf/layer/proxy/http_response.h"

#define HTTP_PARSER_C_CALLBACK_NAME(method) C##method##Cb

#define HTTP_PARSER_C_CALLBACK_DEF(method) \
  static int HTTP_PARSER_C_CALLBACK_NAME(method)(http_parser * p_parser)

#define HTTP_PARSER_C_DATA_CALLBACK_DEF(method)                                \
  static int HTTP_PARSER_C_CALLBACK_NAME(method)(http_parser*, const char* at, \
                                                 size_t length)

namespace ssf {
namespace layer {
namespace proxy {
namespace detail {

class HttpResponseBuilder {
 private:
  enum ParserStatus : int {
    kParserError = -1,
    kParserOk = 0,
    kParserNoBody = 1
  };

 public:
  HttpResponseBuilder();

  inline bool complete() { return complete_; }

  ParserStatus ProcessInput(const char* p_data, std::size_t size);

  HttpResponse Get();

  void Reset();

 private:
  ParserStatus OnMessageBegin(http_parser* p_parser);
  ParserStatus OnStatus(http_parser*, const char* at, size_t length);
  ParserStatus OnHeaderName(http_parser*, const char* at, size_t length);
  ParserStatus OnHeaderValue(http_parser*, const char* at, size_t length);
  ParserStatus OnHeadersComplete(http_parser* p_parser);
  ParserStatus OnBody(http_parser*, const char* at, size_t length);
  ParserStatus OnMessageComplete(http_parser* p_parser);

  // C parser callbacks
  HTTP_PARSER_C_CALLBACK_DEF(OnMessageBegin);
  HTTP_PARSER_C_DATA_CALLBACK_DEF(OnStatus);
  HTTP_PARSER_C_DATA_CALLBACK_DEF(OnHeaderName);
  HTTP_PARSER_C_DATA_CALLBACK_DEF(OnHeaderValue);
  HTTP_PARSER_C_CALLBACK_DEF(OnHeadersComplete);
  HTTP_PARSER_C_DATA_CALLBACK_DEF(OnBody);
  HTTP_PARSER_C_CALLBACK_DEF(OnMessageComplete);

 private:
  http_parser parser_;
  http_parser_settings parser_settings_;

  HttpResponse response_;

  bool complete_;
  bool processing_header_name_;
  std::string current_header_name_;
  std::string current_header_value_;
  std::stringstream ss_body_;
};

}  // detail
}  // proxy
}  // layer
}  // ssf

#endif  // SSF_LAYER_PROXY_HTTP_RESPONSE_BUILDER_H_