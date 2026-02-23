#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "router.hpp"

std::string get_mime_type(const std::string& path) {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return "application/octet-stream";
    
    std::string ext = path.substr(dot + 1);

    if (ext == "html") return "text/html";
    if (ext == "css")  return "text/css";
    if (ext == "js")   return "application/javascript";
    if (ext == "png")  return "image/png";
    if (ext == "jpg")  return "image/jpeg";
    if (ext == "jpeg") return "image/jpeg";
    if (ext == "ico")  return "image/x-icon";
    if (ext == "txt")  return "text/plain";

    return "application/octet-stream";
}

std::string route_request(const HTTPRequest& req) {
    // Crafts response
    std::string status;
    std::string body;

    std::string path = req.path;
    std::string method = req.method;

    // Default to index.html
    if (path == "/") {
        path = "/index.html";
    }

    // Ensure no directory traversal
    if (path.find("..") != std::string::npos) {
        status = "403 Forbidden";
        body = "403 Forbidden";
    } else {
        if (method == "GET") {
            const std::string DOC_ROOT = "static";
            std::string file_path = DOC_ROOT + path;
            std::ifstream file(file_path, std::ios::binary);

            if (!file.is_open()) {
                std::cerr << "Failed to open: " << file_path << "\n";
                status = "404 Not Found";
                body = "404 Not Found";
            } else {
                std::stringstream bufferStream;
                bufferStream << file.rdbuf();
                body = bufferStream.str();
                file.close();

                status = "200 OK";
            }

        } else if (method == "POST") {
            status = "200 OK";
            body = "POST request";
        } else if (method == "PUT") {
            status = "200 OK";
            body = "PUT request";
        } else if (method == "DELETE") {
            status = "200 OK";
            body = "DELETE request";
        } else {
            status = "405 Method Not Allowed";
            body = "405 Method Not Allowed";
        }
    }

    std::string content_type = get_mime_type(path);

    std::string response =
        "HTTP/1.1 " + status + "\r\n"
        "Content-Type: " + content_type + "\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n" +
        "\r\n" +
        body;

    return response;
}
