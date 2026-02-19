#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdexcept>
#include <errno.h>
#include <ctime>
#include <thread>
#include <map>
#include <algorithm>

#define MAX_REQ 8192

// TODO
// 1. CREATE HTTP_REQUEST STRUCT
// 2. ADD HELPER FUNCS FOR handle_client()
// 3. SERVE ARBITRARY STATIC FILES
// 4. ADD MIME TYPE DETECTION
// 5. ADD REQUEST LOGGING
// 6. PATCH SECURITY HOLES
// 7. REFACTOR + CLEAN UP
// 8. DOCUMENT AND MOVE ON
//

struct HTTPRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
};

typedef enum {
    ERR_413,
    ERR_408,
    ERR_CLOSED,
    SUCCESS
}
FDOPOutcomes;

FDOPOutcomes read_http_request(int fd, char* req);
int set_up_server_socket(int port);
void send_error(int client_fd, const char* status);
void handle_client(int client_fd);
FDOPOutcomes write_response(int client_fd, std::string response);
bool parse_request(const std::string& raw, HTTPRequest& req);
std::string route_request(const HTTPRequest& req);

int main() {
    std::cout << "HTTP server starting..." << std::endl;

    int server_fd = set_up_server_socket(8080);

    std::cout << "The server is running!\n";

    // Initializes a structure to contain client socket address info
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (true) {
        // Accepts a connection from a client and returns a file descriptor representing the connection
        int client_fd = accept(server_fd, (sockaddr*) & client_addr, & client_len);
        if (client_fd < 0) continue;

        std::thread t(handle_client, client_fd);
        t.detach();
    }

    // Closes server connection
    close(server_fd);

    return 0;
}

FDOPOutcomes read_http_request(int fd, char* req) {
    size_t total = 0;

    int content_length = 0;
    int headers_done = 0;
    char * body_start = NULL;
    while (total < MAX_REQ) {
        ssize_t n = read(fd, req + total, MAX_REQ - total);
        
        if (n > 0) {
            total += n;
            if (total < MAX_REQ) req[total] = '\0'; // safe for strstr()

            // Did we receive a full header?
            if (!headers_done) {
                char* end = strstr(req, "\r\n\r\n");
                if (end) {
                    headers_done = 1;
                    body_start = end + 4;

                    // Do we have a content length?
                    char* cl = strstr(req, "Content-Length:");
                    if (cl) {
                        cl += 15;
                        while ( * cl == ' ') cl++;
                        content_length = atoi(cl);
                    } else {
                        content_length = 0;
                    }

                    if (content_length == 0) break;
                }
            }

            if (total >= MAX_REQ) return ERR_413;

            // How many body bytes do we have?
            if (headers_done && content_length > 0) {
                size_t body_read = total - (body_start - req);
                if (body_read >= (size_t) content_length) {
                    break;
                }
            }
        } else if (n == 0) {
            return ERR_CLOSED;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return ERR_408;
            } else {
                return ERR_CLOSED;
            }
        }
    }

    return SUCCESS;
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
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, & opt, sizeof(opt));

    // Initializes a structure to contain server socket address info
    struct sockaddr_in s;

    // Assigns necessary values to each field in socket address struct
    s.sin_family = AF_INET;
    s.sin_addr.s_addr = INADDR_ANY;
    s.sin_port = htons(port);

    // Binds socket to specified address
    int bind_result = bind(server_fd, (struct sockaddr * ) & s, sizeof(s));
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

    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, & timeout, sizeof(timeout));

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

FDOPOutcomes write_response(int client_fd, std::string response) {
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
            std::string file_path = "static" + path;
            std::ifstream file(file_path);

            if (!file.is_open()) {
                std::cerr << "Failed to open index.html\n";
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

    std::string response =
        "HTTP/1.1 " + status + "\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n" +
        "\r\n" +
        body;

    return response;
}
