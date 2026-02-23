#ifndef ROUTER_HPP
#define ROUTER_HPP


#include <string>
#include "http_request.hpp"


std::string route_request(const HTTPRequest& req);
std::string get_mime_type(const std::string& path);


#endif
