#ifndef SOCKET_UTILS_HPP
#define SOCKET_UTILS_HPP


#include <cstddef>
#include <string>


constexpr size_t MAX_REQ = 8192;


enum FDOPOutcomes {
    ERR_400,
    ERR_413,
    ERR_408,
    ERR_CLOSED,
    SUCCESS
};


FDOPOutcomes read_http_request(int fd, char* req);
int set_up_server_socket(int port);
void send_error(int client_fd, const char* status);
void handle_client(int client_fd, const std::string& client_ip);
FDOPOutcomes write_response(int client_fd, const std::string& response);
std::string get_timestamp();


#endif
