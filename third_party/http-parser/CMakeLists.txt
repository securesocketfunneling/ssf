add_library(http_parser STATIC http-parser/http_parser.h http-parser/http_parser.c)
target_include_directories(http_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_property(TARGET http_parser PROPERTY FOLDER "Third party")
