#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "../common/structures.h"
#include "../common/helper.h"
#include "functionalities.h"
#include <arpa/inet.h>
#include <pthread.h>

#define BACKLOG 10 // Maximum length of the queue of pending connections

// Forward declarations of the functions
struct sockaddr_in get_server_addr(in_port_t port);
struct Command parse_command(const char *message);
void exec_command(int *sock_fd, struct Command command);
// void response_error(const char *message);
char *format_response(const char *code, const char *message);
void send_message(int *sock_fd, char *message);
struct recv_msg_t recv_message_server(int *sock_fd);

// Assuming fs_mount and fs_unmount are provided
// extern void fs_mount(int sock_fd);
// extern void fs_unmount(fs);

// Command type enumeration

void init_storage_server(struct storage_server *server, int port, int ss_id)
{

    // keep localhost ip
    inet_pton(AF_INET, "127.0.0.1", &server->ip_of_ss);
    server->port_of_ss = port; // Ports 10000, 10001, 10002
    // Other initializations...
    server->last_backup_itself_at = 0;
    server->last_backup_of_other_at = 0;
    server->working = 1;
    server->backup_memory_consumed[0] = 0; // for 0th server it is 1. // for 1st server it is 0. // for 2nd server it is 0.
    server->backup_memory_consumed[1] = 0; // for 0th server it is 2. // for 1st server it is 2. // for 2nd server it is 1.
    //  x%3 == 0. then  +1,+2; x%3 == 1. then  -1,+1;  x%3 == 2. then  -2,-1;
    server->backup_of[0] = 1;      // => need to frame up in a generalized way.
    server->backup_of[1] = 2;      // => need to frame up in a generalized way.
    server->backuped_up_in[0] = 0; // => need to frame up in a generalized way.
    server->backuped_up_in[1] = 1; // => need to frame up in a generalized way.
    server->current_storage_capacity = 0;
    server->num_of_files = 0;
    server->id = ss_id;

    // brute forcing for now
    server->files[0] = "file1";
}

void *handle_client(void *client_sock)
{
    int *new_sock = (int *)client_sock;
    if (new_sock < 0)
    {
        perror("Error: Failed to accept socket connection");
    }
    printf("new_sock: %d\n", *new_sock);
    while (1)
    {
        struct recv_msg_t msg;
        msg.quit = 0;
        printf("Waiting for message from nfs\n");
        msg = recv_message_server(new_sock);
        if (msg.quit)
        {
            printf("quitting\n");
            break;
        }
        struct Command command = parse_command(msg.message);

        printf("command.type: %d\n", command.type);
        if (command.type == invalid_cmd)
        {
            char *response = format_response(command.data, command.data);
            send_message(new_sock, response);
            free(response);
        }
        else if (command.type == noop_cmd)
        {
            char *response = format_response("200 OK", "");
            send_message(new_sock, response);
            free(response);
        }
        else
        {
            exec_command(new_sock, command);
            // search_and_send_to_storage_server(new_sockfd, &fs, command, msg.message);
        }
        msg.quit = 0;
    }
    close(*new_sock);
}
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: ./storage_server port#\n");
        return -1;
    }
    int port = atoi(argv[1]);
    int ss_id = atoi(argv[2]);
    printf("port: %d\n", port);
    printf("ss_id: %d\n", ss_id);
    struct storage_server server;
    init_storage_server(&server, port, ss_id);
    int sockfd = socket(PF_INET, SOCK_STREAM, PF_UNSPEC);
    if (sockfd < 0)
    {
        perror("Error: Failed to create socket");
        exit(1);
    }

    int opts = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opts, sizeof(opts));

    struct sockaddr_in server_addr = get_server_addr(port);
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error: Failed to bind to port");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) < 0)
    {
        perror("Error: Failed to listen for connections");
        exit(1);
    }

    printf("Server is ready to handle connections on port %d\n", port);

    while (1)
    {
        printf("Creating Connection\n");
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &server.ip_of_ss, ip_str, INET_ADDRSTRLEN);
        printf("Server IP: %s, Port: %d\n", ip_str, server.port_of_ss);
        struct sockaddr_in client_addr;
        socklen_t client_len;
        printf("Waiting for connection\n");

        while (1)
        {
            int new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
            printf("new_sockfd: %d\n", new_sockfd);
            if (new_sockfd < 0)
            {
                perror("Error: Failed to accept socket connection");
                continue;
            }
            pthread_t client_thread;
            if (pthread_create(&client_thread, NULL, (void *)handle_client, (void *)&new_sockfd) < 0)
            {
                perror("Error: Failed to create client thread");
                continue;
            }
        }
    }

    close(sockfd);
    return 0;
}

#include <strings.h>    // For bzero
#include <netinet/in.h> // For sockaddr_in, in_port_t, htons, htonl

struct sockaddr_in get_server_addr(in_port_t port)
{
    struct sockaddr_in server_addr;
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = PF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    return server_addr;
}

#include <string.h>
#include <sys/socket.h> // For send()

// extern void fs_mkdir(const char *file);
// extern void fs_ls();
// extern void fs_cd(const char *file);
// extern void fs_home();
// extern void fs_rmdir(const char *file);
// extern void fs_append(const char *file, const char *data);
// extern void fs_stat(const char *file);
// extern void fs_head(const char *file, int n);
// extern void fs_rm(const char *file);

void exec_command(int *socket_fd, struct Command command)
{
    switch (command.type)
    {
    case mkdir_cmd:
        fs_mkdir(socket_fd, command.file);
        break;
    case mkfile_cmd:
        fs_mkfile(socket_fd, command.file);
        break;
    case ls_cmd:
        // fs_ls();
        break;
    case cd_cmd:
        // fs_cd(command.file);
        break;
    case home_cmd:
        // fs_home();
        break;
    case rmdir_cmd:
        fs_rmdir(socket_fd, command.file);
        break;
    case create_cmd:
        fs_create(socket_fd, command.file);
        break;
    case write_cmd:
        fs_write(socket_fd, command.file, command.data);
        // fs_append(command.file, command.data);
        break;
    case stat_cmd:
        fs_stat(socket_fd, command.file);
        break;
    case cat_cmd:
        fs_cat(socket_fd, command.file);
        break;
    case cp_cmd:
        // fs_head(command.file, atoi(command.data));
        break;
    case rm_cmd:
        fs_rm(socket_fd, command.file);
        break;
    case quit_cmd:
        // TODO: Call command type doesn't exist error which should be defined in Wrapped_NFS.h
        break;
    // ! also implement cases for noop_cmd and invalid_cmd
    default:
        // TODO: Call invalid command error which should be defined in Wrapped_NFS.h
        break;
    }
}

// Assuming enum CommandType and struct Command are already defined
