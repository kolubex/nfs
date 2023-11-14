#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define BACKLOG 10 // Maximum length of the queue of pending connections

// Forward declarations of the functions
struct sockaddr_in get_server_addr(in_port_t port);
struct Command parse_command(const char* message);
void exec_command(int sock_fd, struct FileSys* fs, struct Command command);
void response_error(const char* message);
char* format_response(const char* code, const char* message);
void send_message(int sock_fd, const char* message);
struct recv_msg_t recv_message_server(int sock_fd);

// Assuming the existence of a FileSys structure
struct FileSys {
    // Add necessary fields here
    int fs_sock;
    char* current_dir;
};

// Assuming fs_mount and fs_unmount are provided
extern void fs_mount(struct FileSys* fs, int sock_fd);
extern void fs_unmount(struct FileSys* fs);

// Command type enumeration
enum CommandType {
    mkdir_cmd,
    ls_cmd,
    cd_cmd,
    home_cmd,
    rmdir_cmd,
    create_cmd,
    append_cmd,
    stat_cmd,
    cat_cmd,
    head_cmd,
    rm_cmd,
    quit_cmd,
    noop_cmd,
    invalid_cmd
};

// Command structure
struct Command {
    enum CommandType type;
    char file[256]; // Fixed-size buffer for filename
    char data[256]; // Fixed-size buffer for data
};

// Assuming recv_msg_t is defined as follows
struct recv_msg_t {
    char* message;
    int quit;
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./nfsserver port#\n");
        return -1;
    }
    int port = atoi(argv[1]);

    int sockfd = socket(PF_INET, SOCK_STREAM, PF_UNSPEC);
    if (sockfd < 0) {
        perror("Error: Failed to create socket");
        exit(1);
    }

    int opts = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opts, sizeof(opts));

    struct sockaddr_in server_addr = get_server_addr(port);
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error: Failed to bind to port");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) < 0) {
        perror("Error: Failed to listen for connections");
        exit(1);
    }

    printf("Server is ready to handle connections\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len;
        int new_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (new_sockfd < 0) {
            perror("Error: Failed to accept socket connection");
            continue;
        }

        struct FileSys fs;
        fs_mount(&fs, new_sockfd);

        struct recv_msg_t msg;
        msg.quit = 0;
        while (!msg.quit) {
            msg = recv_message_server(new_sockfd);
            if (msg.quit) {
                break;
            }

            struct Command command = parse_command(msg.message);
            if (command.type == invalid_cmd) {
                char* response = format_response(command.data, command.data);
                send_message(new_sockfd, response);
                free(response);
            } else if (command.type == noop_cmd) {
                char* response = format_response("200 OK", "");
                send_message(new_sockfd, response);
                free(response);
            } else {
                exec_command(new_sockfd, fs, command);
            }
        }

        close(new_sockfd);
        fs_unmount(&fs);
    }

    close(sockfd);
    return 0;
}

#include <strings.h> // For bzero
#include <netinet/in.h> // For sockaddr_in, in_port_t, htons, htonl

struct sockaddr_in get_server_addr(in_port_t port) {
    struct sockaddr_in server_addr;
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = PF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    return server_addr;
}


extern void fs_mkdir(struct FileSys* fs, const char* file);
extern void fs_ls(struct FileSys* fs);
extern void fs_cd(struct FileSys* fs, const char* file);
extern void fs_home(struct FileSys* fs);
extern void fs_rmdir(struct FileSys* fs, const char* file);
extern void fs_create(struct FileSys* fs, const char* file);
extern void fs_append(struct FileSys* fs, const char* file, const char* data);
extern void fs_stat(struct FileSys* fs, const char* file);
extern void fs_cat(struct FileSys* fs, const char* file);
extern void fs_head(struct FileSys* fs, const char* file, int n);
extern void fs_rm(struct FileSys* fs, const char* file);


void exec_command(int socket_fd, struct FileSys* fs, struct Command command) {


    switch (command.type) {
        case mkdir_cmd:
            fs_mkdir(fs, command.file);
            break;
        case ls_cmd:
            fs_ls(fs);
            break;
        case cd_cmd:    
            fs_cd(fs, command.file);
            break;
        case home_cmd:  
            fs_home(fs);
            break;
        case rmdir_cmd: 
            fs_rmdir(fs, command.file);
            break;
        case create_cmd:
            fs_create(fs, command.file);
            break;
        case append_cmd:
            fs_append(fs, command.file, command.data);
            break;
        case stat_cmd:  
            fs_stat(fs, command.file);
            break;
        case cat_cmd:   
            fs_cat(fs, command.file);
            break;
        case head_cmd:
            fs_head(fs, command.file, atoi(command.data));
            break;
        case rm_cmd:
            fs_rm(fs, command.file);
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

struct Command parse_command(const char* message) {
    struct Command cmd;
    cmd.file[0] = NULL;
    cmd.data[0] = NULL;

    char name[256]; // Buffer for the command name
    char file[256]; // Buffer for the file name
    char data[256]; // Buffer for data or number

    // Initialize these to empty strings
    name[0] = file[0] = data[0] = '\0';

    int tokens = sscanf(message, "%255s %255s %255s", name, file, data);

    // Token counts and parsing logic
    if (strcmp(name, "mkdir") == 0) {
        cmd.type = mkdir_cmd;
    }
    if (strcmp(name, "ls") == 0) {
        cmd.type = ls_cmd;
    }
    if (strcmp(name, "cd") == 0) {
        cmd.type = cd_cmd;
    }
    if (strcmp(name, "home") == 0) {
        cmd.type = home_cmd;
    }
    if (strcmp(name, "rmdir") == 0) {
        cmd.type = rmdir_cmd;
    }
    if (strcmp(name, "create") == 0) {
        cmd.type = create_cmd;
    }
    if (strcmp(name, "append") == 0) {
        cmd.type = append_cmd;
    }
    if (strcmp(name, "stat") == 0) {
        cmd.type = stat_cmd;
    }
    if (strcmp(name, "cat") == 0) {
        cmd.type = cat_cmd;
    }
    if (strcmp(name, "head") == 0) {
        cmd.type = head_cmd;
    }
    if (strcmp(name, "rm") == 0) {
        cmd.type = rm_cmd;
    }
    if (strcmp(name, "quit") == 0) {
        cmd.type = quit_cmd;
    }
    else {
        // Handle invalid or noop
        if (tokens == 0) {
            cmd.type = noop_cmd;
        } else {
            cmd.type = invalid_cmd;
            // In case of invalid command, overwrite data with an error message
            strncpy(cmd.data, "Invalid command: Unknown command", 255);
            cmd.data[255] = '\0'; // Ensure null-termination
        }
    }

    // DO ABOVE COMMENTED IMPLEMENTATION IN C
    enum CommandType type = cmd.type;
    if (type == ls_cmd || type == home_cmd || type == quit_cmd) {
        if (tokens != 1) {
            cmd.type = invalid_cmd;
            strncpy(cmd.data, "Invalid command: not enough arguments. Requires 1 token", 255);
            cmd.data[255] = '\0';
        }
    } else if (type == mkdir_cmd || type == cd_cmd || type == rmdir_cmd || type == create_cmd || type == cat_cmd || type == rm_cmd || type == stat_cmd) {
        if (tokens != 2) {
            cmd.type = invalid_cmd;
            strncpy(cmd.data, "Invalid command: not enough arguments. Requires 2 tokens", 255);
            cmd.data[255] = '\0';
        }
    } else if (type == append_cmd || type == head_cmd) {
        if (tokens != 3) {
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