#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP


#include <string>
#include "http_request.hpp"


bool parse_request(const std::string& raw, HTTPRequest& req);


#endif
