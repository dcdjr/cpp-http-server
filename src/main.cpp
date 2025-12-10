#include <iostream>
#include <string>
#include <cstring>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <ctime>

int main() {
    std::cout << "HTTP server starting..." << std::endl;

    // Initializes socket and ensures no errors during initialization
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "There was an error initializing the socket.\n";
        return 1;
    }

    // Enable SO_REUSEADDR so server OS allows the port to be reused
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Initializes a structure to contain server socket address info
    struct sockaddr_in s = {0};

    // Assigns necessary values to each field in socket address struct
    s.sin_family = AF_INET;
    s.sin_addr.s_addr = INADDR_ANY;
    s.sin_port = htons(8080);

    // Binds socket to specified address
    int bind_result = bind(server_fd, (struct sockaddr*)&s, sizeof(s));
    if (bind_result < 0) {
        std::cerr << "There was an error binding the socket.\n";
        return 1;
    }

    // Make a call to listen on the socket while checking for errors
    int backlog = 10;
    int listen_result = listen(server_fd, backlog);
    if (listen_result < 0) {
        std::cerr << "There was an error attempting to listen on the socket.\n";
        return 1;
    }

    std::cout << "The server is running!\n";

    // Initializes a structure to contain client socket address info
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Accepts a connection from a client and returns a file descriptor representing the connection
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        std::cerr << "There was an error accepting the connection from the client.\n";
        return 1;
    }

    std::cout << "Client connected!\n";

    // Stores client HTTP request in a buffer of size 2MB
    char buffer[2048];
    int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        std::cerr << "There was an error reading bytes from the client connection.\n";
        return 1;
    }
    // Null terminate HTTP request
    buffer[bytes_read] = '\0';

    std::cout << "Raw request:\n" << buffer << "\n";

    // Convert buffer to std::string
    std::string request_str = buffer;

    // Find the first \r\n (end of request line)
    size_t end_of_line = request_str.find("\r\n");
    if (end_of_line == std::string::npos) {
        std::cerr << "Malformed request: missing request line.\n";
        return 1;
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

    // Check path
    std::string body;

    if (path == "/") {
        body = "Hello from root!";
    } else if (path == "/hello") {
        body = "Hello there!";
    } else if (path == "/time") {
        std::time_t now = std::time(nullptr);
        std::string t = std::ctime(&now);

        // ctime() puts a newline at the end, remove it
        if (!t.empty() && t.back() == '\n') {
            t.pop_back();
        }
        
        body = "Current server time: " + t;
    } else {
        body = "404 Not Found";
    }

    // Crafts response
    std::string response;

    if (body != "404 Not Found") {
        response = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "\r\n" + 
            body;
    } else {
        response =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "404 Not Found";
    }


    // Writes crafted response to file descriptor representing client connection
    write(client_fd, response.c_str(), response.size());

    // Closes client and server connection
    close(client_fd);
    close(server_fd);

    return 0;
}