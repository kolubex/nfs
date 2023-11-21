#ifndef FILESYS_H
#define FILESYS_H
#include "structures.h"

struct recv_msg_t
{
    char *message; // Assuming maximum message size
    int quit;
};

char *format_response(const char *code, const char *message);
void send_message(int *sock_fd, char *message);
struct recv_msg_t recv_message_client(int sock_fd);
struct recv_msg_t recv_message_server(int *sock_fd);
int get_socket(char *ip_address, int port_num);
struct Command parse_command(const char *message);

#endif
