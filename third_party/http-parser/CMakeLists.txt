add_target(http_parser
  TYPE library ${EXEC_FLAG} STATIC
  FILES
    http-parser/http_parser.h
    http-parser/http_parser.c
  INCLUDE_DIRECTORIES PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  HEADER_FILTER   "\\.h(h|m|pp|xx|\\+\\+)?"
)

project_group("Third party" http_parser)
