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

#define MAX_REQ 8192

typedef enum {
    ERR_413,
    ERR_408,
    ERR_CLOSED,
    SUCCESS
} ReadHTTPOutcomes;

ReadHTTPOutcomes read_http_request(int fd, char* req);
int set_up_server_socket(int port);
void send_error(int client_fd, const char* status);

int main() {
    std::cout << "HTTP server starting..." << std::endl;
        
    int server_fd = set_up_server_socket(8080);

    std::cout << "The server is running!\n";

    // Initializes a structure to contain client socket address info
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (true) {
        // Accepts a connection from a client and returns a file descriptor representing the connection
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            std::cerr << "There was an error accepting the connection from the client.\n";
            return 1;
        }
        
        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        std::cout << "Client connected!\n";
        
        char req[MAX_REQ] = {0};

        ReadHTTPOutcomes request_read_outcome = read_http_request(client_fd, req);

        switch (request_read_outcome) {
            case ERR_408:
                send_error(client_fd, "408 Request Timeout");
                close(client_fd);
                continue;
            case ERR_413:
                send_error(client_fd, "413 Payload Too Large");
                close(client_fd);
                continue;
            case ERR_CLOSED:
                close(client_fd);
                continue;
            case SUCCESS:
                break;
            default:
                close(client_fd);
                continue;
        }

        std::cout << "Raw request:\n" << req << "\n";

        // Convert buffer to std::string
        std::string request_str = req;

        // Find the first \r\n (end of request line)
        size_t end_of_line = request_str.find("\r\n");
        if (end_of_line == std::string::npos) {
            std::cerr << "Malformed request: missing request line.\n";
            continue;
        }

        // Extract request line
        std::string request_line = request_str.substr(0, end_of_line);

        // Print request
        std::cout << "Request line: " << request_line << "\n";

        // Find first space
        size_t first_space = request_line.find(' ');

        // Find second space
        size_t second_space = request_line.find(' ', first_space + 1);

        // Extract the pieces
        std::string method = request_line.substr(0, first_space);
        std::string path = request_line.substr(first_space + 1, second_space - (first_space + 1));// (3 + 1, 5 - (3 + 1))
        std::string version = request_line.substr(second_space + 1);

        // Print them
        std::cout << "Method: " << method << "\n";
        std::cout << "Path: " << path << "\n";
        std::cout << "Version: " << version << "\n";

        // Crafts response
        std::string status;
        std::string body;

        if (method == "GET" && path == "/") {
            // Serve index.html
            std::cout << "we got here!" << "\n"; 
            std::ifstream file("static/index.html");

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

        std::string response =
            "HTTP/1.1 " + status + "\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "\r\n" +
            body;

        // Writes crafted response to file descriptor representing client connection
        int bytes_written = write(client_fd, response.c_str(), response.size());
        if (bytes_written < 0) {
            std::cerr << "There was an error sending client response.\n";
            return 1;
        }

        // Closes client connection
        close(client_fd);
    }

    // Closes server connection
    close(server_fd);

    return 0;
}

ReadHTTPOutcomes read_http_request(int fd, char* req) {
    size_t total = 0;

    int content_length = 0;
    int headers_done = 0;
    char* body_start = NULL;

    while (total < MAX_REQ) {
        ssize_t n = read(fd, req + total, MAX_REQ - total);
        if (n == 0) return ERR_CLOSED;

        if (n < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                return ERR_408;
            }
        }

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
                    while (*cl == ' ') cl++;
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
            if (body_read >= (size_t)content_length) {
                break;
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
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Initializes a structure to contain server socket address info
    struct sockaddr_in s = {0};

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

    int bytes_written = write(client_fd, response.c_str(), response.size());
    if (bytes_written < 0) {
        std::cerr << "There was an error sending client response.";
    }
}
