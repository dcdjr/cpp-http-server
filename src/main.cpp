#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctime>
#include <thread>

#include "socket_utils.hpp"

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
        if (client_fd < 0) continue;

        std::string client_ip = inet_ntoa(client_addr.sin_addr);

        std::thread t(handle_client, client_fd, client_ip);
        t.detach();
    }

    // Closes server connection
    close(server_fd);

    return 0;
}




