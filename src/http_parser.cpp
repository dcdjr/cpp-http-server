#include <string>
#include <sstream>
#include <algorithm>

#include "http_parser.hpp"

bool parse_request(const std::string& raw, HTTPRequest& req) {
    // Clear headers before filling to prevent stale data is struct is reused
    req.headers.clear();
    req.body.clear();

    // Find the first \r\n (end of request line)
    size_t end_of_line = raw.find("\r\n");
    if (end_of_line == std::string::npos) return false;

    // Extract request line
    std::string request_line = raw.substr(0, end_of_line);

    size_t header_end = raw.find("\r\n\r\n");
    if (header_end == std::string::npos) return false;

    // Find first space
    size_t first_space = request_line.find(' ');
    if (first_space == std::string::npos) return false;

    // Find second space
    size_t second_space = request_line.find(' ', first_space + 1);
    if (second_space == std::string::npos) return false;

    // Extract the pieces
    req.method = request_line.substr(0, first_space);
    req.path = request_line.substr(first_space + 1, second_space - (first_space + 1)); // (3 + 1, 5 - (3 + 1))
    req.version = request_line.substr(second_space + 1);
    
    // Extract headers
    std::string header_block = raw.substr(0, header_end);

    std::istringstream stream(header_block);
    std::string line;

    std::getline(stream, line);

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line.empty())
            break;

        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;

        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        // trim leading space in value
        while (!value.empty() && value.front() == ' ')
            value.erase(0, 1);

        // lowercase key (HTTP headers are case-insensitive)
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        req.headers[key] = value;
    }

    // Extract body (+4 is to account for including \r\n\r\n in body)
    req.body = raw.substr(header_end + 4);

    return true;
}
