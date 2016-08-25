cmake_minimum_required(VERSION 2.8)

set(HTTP_PARSER_FILES
  http-parser/http_parser.h
  http-parser/http_parser.c)

add_target("lib_http_parser"
  TYPE
    library ${EXEC_FLAG} STATIC
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
  FILES
    ${HTTP_PARSER_FILES}
)

target_include_directories(lib_http_parser
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

project_group("Third party" lib_http_parser)
