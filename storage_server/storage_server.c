#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "structures.h"
#include "functionalities.h"
#include <arpa/inet.h>

#define BACKLOG 10 // Maximum length of the queue of pending connections

// Forward declarations of the functions
struct sockaddr_in get_server_addr(in_port_t port);
struct Command parse_command(const char *message);
void exec_command(int sock_fd, struct Command command);
// void response_error(const char *message);
char *format_response(const char *code, const char *message);
void send_message(int sock_fd, const char *message);
struct recv_msg_t recv_message_server(int sock_fd);

// Assuming fs_mount and fs_unmount are provided
// extern void fs_mount(int sock_fd);
// extern void fs_unmount(fs);

// Command type enumeration
enum CommandType
{
    mkdir_cmd,
    ls_cmd,
    cd_cmd,
    home_cmd,
    rmdir_cmd,
    create_cmd,
    write_cmd,
    stat_cmd,
    cat_cmd,
    head_cmd,
    rm_cmd,
    quit_cmd,
    noop_cmd,
    invalid_cmd
};

// Command structure
struct Command
{
    enum CommandType type;
    char file[256]; // Fixed-size buffer for filename
    char data[256]; // Fixed-size buffer for data
};

// Assuming recv_msg_t is defined as follows
struct recv_msg_t
{
    char *message;
    int quit;
};

void init_storage_server(struct storage_server *server, int port, int ss_id)
{

    // keep localhost ip
    inet_pton(AF_INET, "127.0.0.1", &server->ip_of_ss);
    server->port_of_ss = port; // Ports 10000, 10001, 10002
    // Other initializations...
    server->last_backup_itself_at = 0;
    server->last_backup_of_other_at = 0;
    server->working = true;
    server->modified = false;
    server->backup_memory_consumed[0] = 0; // for 0th server it is 1. // for 1st server it is 0. // for 2nd server it is 0.
    server->backup_memory_consumed[1] = 0; // for 0th server it is 2. // for 1st server it is 2. // for 2nd server it is 1.
    //  x%3 == 0. then  +1,+2; x%3 == 1. then  -1,+1;  x%3 == 2. then  -2,-1;
    server->backup_of[0] = 1;      // => need to frame up in a generalized way.
    server->backup_of[1] = 2;      // => need to frame up in a generalized way.
    server->backuped_up_in[0] = 0; // => need to frame up in a generalized way.
    server->backuped_up_in[1] = 1; // => need to frame up in a generalized way.
    server->current_storage_capacity = 0;
    server->num_of_files = 0;
    server->num_of_dirs = 0;
    server->id = ss_id;

    // brute forcing for now
    server->files[0] = "file1";
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
        int new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        printf("new_sockfd: %d\n", new_sockfd);
        if (new_sockfd < 0)
        {
            perror("Error: Failed to accept socket connection");
            continue;
        }
        struct recv_msg_t msg;
        msg.quit = 0;
        while (1)
        {
            printf("Waiting for message from nfs\n");
            msg = recv_message_server(new_sockfd);
            if (msg.quit)
            {
                break;
            }
            struct Command command = parse_command(msg.message);

            printf("command.type: %d\n", command.type);
            if (command.type == invalid_cmd)
            {
                char *response = format_response(command.data, command.data);
                send_message(new_sockfd, response);
                free(response);
            }
            else if (command.type == noop_cmd)
            {
                char *response = format_response("200 OK", "");
                send_message(new_sockfd, response);
                free(response);
            }
            else
            {
                exec_command(new_sockfd, command);
                // search_and_send_to_storage_server(new_sockfd, &fs, command, msg.message);
            }
            msg.quit = 0;
        }
        close(new_sockfd);
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

void exec_command(int socket_fd, struct Command command)
{
    switch (command.type)
    {
    case mkdir_cmd:
        printf("command.file: %s\n", command.file);
        fs_mkdir(socket_fd, command.file);
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
        // fs_rmdir(command.file);
        break;
    case create_cmd:
        fs_create(socket_fd, command.file);
        break;
    case write_cmd:
        // fs_append(command.file, command.data);
        break;
    case stat_cmd:
        // fs_stat(command.file);
        break;
    case cat_cmd:
        printf("command.file: %s\n", command.file);
        fs_cat(socket_fd, command.file);
        break;
    case head_cmd:
        // fs_head(command.file, atoi(command.data));
        break;
    case rm_cmd:
        // fs_rm(command.file);
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

struct Command parse_command(const char *message)
{
    struct Command cmd;
    cmd.file[0] = '\0';
    cmd.data[0] = '\0';

    char name[256]; // Buffer for the command name
    char file[256]; // Buffer for the file name
    char data[256]; // Buffer for data or number
    char port[6];
    char ip[16];

    // Initialize these to empty strings
    name[0] = file[0] = data[0] = ip[0] = port[0] = '\0';

    int tokens = sscanf(message, "%255s %255s %255s %15s %5s", name, file, data, ip, port);

    // Token counts and parsing logic
    if (strcmp(name, "mkdir") == 0)
    {
        cmd.type = mkdir_cmd;
    }
    else if (strcmp(name, "ls") == 0)
    {
        cmd.type = ls_cmd;
    }
    else if (strcmp(name, "cd") == 0)
    {
        cmd.type = cd_cmd;
    }
    else if (strcmp(name, "home") == 0)
    {
        cmd.type = home_cmd;
    }
    else if (strcmp(name, "rmdir") == 0)
    {
        cmd.type = rmdir_cmd;
    }
    else if (strcmp(name, "create") == 0)
    {
        cmd.type = create_cmd;
    }
    else if (strcmp(name, "append") == 0)
    {
        cmd.type = write_cmd;
    }
    else if (strcmp(name, "stat") == 0)
    {
        cmd.type = stat_cmd;
    }
    else if (strcmp(name, "cat") == 0)
    {
        cmd.type = cat_cmd;
    }
    else if (strcmp(name, "head") == 0)
    {
        cmd.type = head_cmd;
    }
    else if (strcmp(name, "rm") == 0)
    {
        cmd.type = rm_cmd;
    }
    else if (strcmp(name, "quit") == 0)
    {
        cmd.type = quit_cmd;
    }
    else
    {
        // Handle invalid or noop
        if (tokens == 0)
        {
            cmd.type = noop_cmd;
        }
        else
        {
            cmd.type = invalid_cmd;
            // In case of invalid command, overwrite data with an error message
            strncpy(cmd.data, "Invalid command: Unknown command", 255);
            cmd.data[255] = '\0'; // Ensure null-termination
        }
    }

    // DO ABOVE COMMENTED IMPLEMENTATION IN C
    enum CommandType type = cmd.type;
    if (type == ls_cmd || type == home_cmd || type == quit_cmd)
    {
        if (tokens != 1)
        {
            cmd.type = invalid_cmd;
            strncpy(cmd.data, "Invalid command: not enough arguments. Requires 1 token", 255);
            cmd.data[255] = '\0';
        }
    }
    else if (type == mkdir_cmd || type == cd_cmd || type == rmdir_cmd || type == create_cmd || type == cat_cmd || type == rm_cmd || type == stat_cmd)
    {
        if (tokens != 2)
        {
            cmd.type = invalid_cmd;
            strncpy(cmd.data, "Invalid command: not enough arguments. Requires 2 tokens", 255);
            cmd.data[255] = '\0';
        }
    }
    else if (type == write_cmd || type == head_cmd)
    {
        if (tokens != 3)
        {
            cmd.type = invalid_cmd;
            strncpy(cmd.data, "Invalid command: not enough arguments. Requires 3 token", 255);
            cmd.data[255] = '\0';
        }
    }

    // Copy the file and data buffers into the command structure
    strncpy(cmd.file, file, 255);
    cmd.file[255] = '\0'; // Ensure null-termination
    strncpy(cmd.data, data, 255);
    cmd.data[255] = '\0'; // Ensure null-termination

    return cmd;
}