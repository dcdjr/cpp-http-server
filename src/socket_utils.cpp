#include <iostream>
#include <string>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "http_parser.hpp"
#include "http_request.hpp"
#include "socket_utils.hpp"
#include "router.hpp"

FDOPOutcomes read_http_request(int fd, char* req) {
    size_t total = 0;
    
    // Read headers
    while (total < MAX_REQ) {
        ssize_t n = read(fd, req + total, MAX_REQ - total);

        if (n == 0) return ERR_CLOSED;
        if (n < 0) return ERR_408;

        total += n;
        req[total] = '\0'; // null termination for strstr()

        char* end = strstr(req, "\r\n\r\n");
        if (end) {
            // extract headers
            size_t header_len = end - req + 4;

            std::string raw_headers(req, header_len);

            HTTPRequest request;
            if (!parse_request(raw_headers, request)) {
                return ERR_CLOSED;
            }

            // get content-length
            int content_length = 0;
            if (request.headers.count("content-length")) {
                content_length = std::stoi(request.headers["content-length"]);
            }

            // read body
            size_t body_read = total - header_len;

            while (body_read < (size_t)content_length) {
                ssize_t m = read(fd, req + total, MAX_REQ - total);

                if (m == 0) return ERR_CLOSED;
                if (m < 0) return ERR_408;

                total += m;
                body_read += m;
            }

            return SUCCESS;
        }
    }

    return ERR_413;
}

int set_up_server_socket(int port) {
    // Initializes socket and ensures no errors during initialization
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "There was an error initializing the socket.\n";
        return -1;
    }

    // Enable SO_REUSEADDR so server OS allows the port to be reused quickly    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Initializes a structure to contain server socket address info
    struct sockaddr_in s;

    // Assigns necessary values to each field in socket address struct
    s.sin_family = AF_INET;
    s.sin_addr.s_addr = INADDR_ANY;
    s.sin_port = htons(port);

    // Binds socket to specified address
    int bind_result = bind(server_fd, (struct sockaddr*)&s, sizeof(s));
    if (bind_result < 0) {
        std::cerr << "There was an error binding the socket.\n";
        return -1;
    }

    // Make a call to listen on the socket while checking for errors
    int backlog = 10;
    int listen_result = listen(server_fd, backlog);
    if (listen_result < 0) {
        std::cerr << "There was an error attempting to listen on the socket.\n";
        return -1;
    }

    return server_fd;
}

void send_error(int client_fd, const char* status) {
    size_t len = strlen(status);

    std::string response =
        "HTTP/1.1 " + std::string(status) + "\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: " + std::to_string(len) + "\r\n"
        "\r\n" +
        status;

    int bytes_written = write_response(client_fd, response);
    if (bytes_written != SUCCESS) {
        std::cerr << "There was an error sending client response.";
    }
}

void handle_client(int client_fd) {
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    std::cout << "Client connected!\n";

    char req[MAX_REQ] = {0};

    FDOPOutcomes request_read_outcome = read_http_request(client_fd, req);

    switch (request_read_outcome) {
    case ERR_408:
        send_error(client_fd, "408 Request Timeout");
        close(client_fd);
        return;
    case ERR_413:
        send_error(client_fd, "413 Payload Too Large");
        close(client_fd);
        return;
    case ERR_CLOSED:
        close(client_fd);
        return;
    case SUCCESS:
        break;
    default:
        close(client_fd);
        return;
    }

    std::string request_str = req;

    HTTPRequest request;

    if (!parse_request(request_str, request)) {
        send_error(client_fd, "400 Malformed Request");
        close(client_fd);
        return;
    }

    // Ensure correct HTTP version
    if (request.version != "HTTP/1.1") {
        send_error(client_fd, "505 HTTP Version Not Supported");
        close(client_fd);
        return;
    }

    std::string response = route_request(request);

    // Writes crafted response to file descriptor representing client connection
    // Ensures full response is written
    FDOPOutcomes write_outcome = write_response(client_fd, response);
    if (write_outcome != SUCCESS) {
        std::cerr << "There was an error sending client response.";
    }

    // Closes client connection
    close(client_fd);
}

FDOPOutcomes write_response(int client_fd, const std::string& response) {
    size_t bytes_written = 0;
    while (bytes_written < response.length()) {
        ssize_t result = write(client_fd, response.c_str() + bytes_written, response.size() - bytes_written);
        
        if (result == 0) return ERR_CLOSED;

        if (result < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                return ERR_408;
            }
        }

        bytes_written += result;
    }
    return SUCCESS;
}
