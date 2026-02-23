#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP


#include <string>
#include <map>


struct HTTPRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};


#endif
