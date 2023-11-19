#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "../common/structures.h"
#include "../common/helper.h"

#define BACKLOG 10 // Maximum length of the queue of pending connections
struct nfs_network fs;
// Forward declarations of the functions
struct sockaddr_in get_server_addr(in_port_t port);
struct Command parse_command(const char *message);
// void exec_command(int sock_fd, struct nfs_network* fs, struct Command command);
// void response_error(const char* message);
// char* format_response(const char* code, const char* message);
// void send_message(int sock_fd, const char* message);
// struct recv_msg_t recv_message_server(int sock_fd);

// Assuming fs_mount and fs_unmount are provided
extern void fs_mount(struct nfs_network *fs);
// extern void fs_unmount(struct nfs_network* fs);

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

void search_and_send_to_storage_server(int *socket_fd, struct nfs_network *fs, struct Command *command, char *message)
{
    // Iterate over the file mappings to find the storage server for the given file
    int storage_server_index = -1;
    if (command->type != mkdir_cmd)
    {

        for (int i = 0; i < MAX_FILES; i++)
        {
            // printf("Comparing file name: %s\n", fs->file_mappings[i].file_name);
            if (strcmp(fs->file_mappings[i].file_name, command->file) == 0)
            {
                storage_server_index = fs->file_mappings[i].storage_server_index;
                break;
            }
        }

        if (storage_server_index == -1)
        {
            printf("File not found in any storage server.\n");
            return;
        }

        // Get the socket for the found storage server
        printf("Found storage server index: %d\n", storage_server_index);
        printf("command->type: %d\n", command->type);
    }
    else
    {
        printf("We are NOT entering search\n");
        int ssid;
        int curr_storage = 1e9;
        for (int i = 0; i < MAX_STORAGE_SERVERS; i++)
        {
            if (fs->storage_servers[i].working)
            {
                if (curr_storage > fs->storage_servers[i].current_storage_capacity)
                {
                    curr_storage = fs->storage_servers[i].current_storage_capacity;
                    ssid = i;
                }
            }
        }

        uint32_t ssid_port_number = fs->storage_servers[ssid].port_of_ss;
        uint32_t ssid_ip_address = fs->storage_servers[ssid].ip_of_ss;

        char* ip_address = "127.0.0.1";
        printf("ssid_port_number to which NS is connecting: %d\n", ssid_port_number);
        printf("ssid_ip_address to which NS is connecting: %d\n", ssid_ip_address);

        // create a socket and connect the client with the storage server

        int storage_server_socket = get_socket(ip_address, ssid_port_number);
        send_message(&storage_server_socket, message);
        printf("Sent message from the SS to the NS");
        close(storage_server_socket);
    }


    if (command->type == cat_cmd || command->type == write_cmd || command->type == stat_cmd || command->type == mkdir_cmd)
    {
        printf("Entered if statement, now send data\n");
        uint32_t storage_server_ip_address = fs->storage_servers[storage_server_index].ip_of_ss;
        char storage_server_ip_string[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &storage_server_ip_address, storage_server_ip_string, INET_ADDRSTRLEN);

        uint16_t storage_server_port_num = fs->storage_servers[storage_server_index].port_of_ss;
        char storage_server_port_string[6]; // Port number can be up to 5 digits + null terminator
        sprintf(storage_server_port_string, "%u", storage_server_port_num);

        int storage_server_id = fs->storage_servers[storage_server_index].id;
        // convert storage_server_id to string with a int to string function
        char storage_server_id_string[10];
        sprintf(storage_server_id_string, "%d", storage_server_id);
        char message[1024]; // Ensure this buffer is large enough
        sprintf(message, "%s %s %s", storage_server_ip_string, storage_server_port_string, storage_server_id_string);
        char *storage_server_info = format_response("200 OK", message);
        send_message(socket_fd, storage_server_info);
    }
    else
    {
        int storage_server_socket = fs->server_sockets[storage_server_index];
        if (storage_server_socket < 0)
        {
            printf("Invalid storage server socket.\n");
            return;
        }
        // Send the message to the found storage server
        printf("Sending message to storage server: %s\n", message);
        printf("storage_server_socket: %d\n", storage_server_socket);
        if (send(storage_server_socket, message, strlen(message), 0) < 0)
        {
            perror("Failed to send message to storage server");
        }
    }
}

void *client_thread(void *client_sock)
{
    int *new_sockfd = (int *)client_sock;
    if (*new_sockfd < 0)
    {
        perror("Error: Failed to accept socket connection");
    }
    printf("Accepted connection from client\n");
    struct recv_msg_t msg;
    msg.quit = 0;
    while (1)
    {
        printf("Created message struct\n");
        msg = recv_message_server(new_sockfd);
        printf("Received message from client: %s\n", msg.message);
        if (msg.quit)
        {
            printf("Client has quit\n");
            break;
        }
        struct Command command = parse_command(msg.message);
        printf("Parsed command\n");
        if (command.type == invalid_cmd)
        {
            printf("Invalid command\n");
            char *response = format_response(command.data, command.data);
            send_message(new_sockfd, response);
            free(response);
        }
        else if (command.type == noop_cmd)
        {
            printf("Noop command\n");
            char *response = format_response("200 OK", "");
            send_message(new_sockfd, response);
            free(response);
        }
        else
        {
            printf("Searching for file in storage servers\n");
            search_and_send_to_storage_server(new_sockfd, &fs, &command, msg.message);
        }
    }

    close(*new_sockfd);
    // fs_unmount(&fs);
}

int main(int argc, char *argv[])
{
    printf("Server is starting up...\n");
    if (argc < 2)
    {
        printf("Usage: ./nfsserver port#\n");
        return -1;
    }
    int port = atoi(argv[1]);

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

    printf("Server is ready to handle connections\n");
    // mount the fs here
    fs_mount(&fs); //! need to edit this functions code, you send request to storage servers to give list of files they have and the compile them to keep it in mapping
    printf("Mounted the file system\n");
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len;
        int new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        pthread_t cli_thread;
        if (pthread_create(&cli_thread, NULL, client_thread, (void *)&new_sockfd) < 0)
        {
            perror("Error: Failed to create thread");
            continue;
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

// extern void fs_mkdir(struct nfs_network* fs, const char* file);
// extern void fs_ls(struct nfs_network* fs);
// extern void fs_cd(struct nfs_network* fs, const char* file);
// extern void fs_home(struct nfs_network* fs);
// extern void fs_rmdir(struct nfs_network* fs, const char* file);
// extern void fs_create(struct nfs_network* fs, const char* file);
// extern void fs_append(struct nfs_network* fs, const char* file, const char* data);
// extern void fs_stat(struct nfs_network* fs, const char* file);
// extern void fs_cat(struct nfs_network* fs, const char* file);
// extern void fs_head(struct nfs_network* fs, const char* file, int n);
// extern void fs_rm(struct nfs_network* fs, const char* file);

// void exec_command(int socket_fd, struct nfs_network* fs, struct Command command) {

//     switch (command.type) {
//         case mkdir_cmd:
//             fs_mkdir(fs, command.file);
//             break;
//         case ls_cmd:
//             fs_ls(fs);
//             break;
//         case cd_cmd:
//             fs_cd(fs, command.file);
//             break;
//         case home_cmd:
//             fs_home(fs);
//             break;
//         case rmdir_cmd:
//             fs_rmdir(fs, command.file);
//             break;
//         case create_cmd:
//             fs_create(fs, command.file);
//             break;
//         case write_cmd:
//             fs_append(fs, command.file, command.data);
//             break;
//         case stat_cmd:
//             fs_stat(fs, command.file);
//             break;
//         case cat_cmd:
//             fs_cat(fs, command.file);
//             break;
//         case head_cmd:
//             fs_head(fs, command.file, atoi(command.data));
//             break;
//         case rm_cmd:
//             fs_rm(fs, command.file);
//             break;
//         case quit_cmd:
//             // TODO: Call command type doesn't exist error which should be defined in Wrapped_NFS.h
//             break;
//         // ! also implement cases for noop_cmd and invalid_cmd
//         default:
//             // TODO: Call invalid command error which should be defined in Wrapped_NFS.h
//             break;
//     }

// }

// Assuming enum CommandType and struct Command are already defined

struct Command parse_command(const char *message)
{
    struct Command cmd;
    cmd.file[0] = '\0';
    cmd.data[0] = '\0';

    char name[256]; // Buffer for the command name
    char file[256]; // Buffer for the file name
    char data[256]; // Buffer for data or number

    // Initialize these to empty strings
    name[0] = file[0] = data[0] = '\0';

    int tokens = sscanf(message, "%255s %255s %255s", name, file, data);
    // print name name, file, data
    printf("name: %s, file: %s, data: %s\n", name, file, data);
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
        printf("cat command\n");
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
            printf("Invalid command after parsing\n");
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