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
#include "LRU.h"
#include "tries.h"

struct TrieNode *root = NULL;
LRUCacheQueue *cacheQueue = NULL;
#define MAX_CACHE
#define BACKLOG 10 // Maximum length of the queue of pending connections
struct nfs_network fs;
// Forward declarations of the functions
struct sockaddr_in get_server_addr(in_port_t port);
struct Command parse_command(const char *message);
int get_socket2(char *ip, int port_num, int ssid);
// void exec_command(int sock_fd, struct nfs_network* fs, struct Command command);
// void response_error(const char* message);
// char* format_response(const char* code, const char* message);
// void send_message(int sock_fd, const char* message);
// struct recv_msg_t recv_message_server(int sock_fd);

// Assuming fs_mount and fs_unmount are provided
extern void fs_mount(struct nfs_network *fs);
void copy_execution(int *socket_fd, struct nfs_network *fs, struct Command *command, char *message, int storage_server_index_read, int storage_server_index_write);
void backup_things(int ssid, char *message);
// extern void fs_unmount(struct nfs_network* fs);

// Command type enumeration
int get_socket2(char *ip, int port_num, int ssid)
{
    if (fs.storage_servers[ssid].working == 0)
    {
        // try for the first backup
        if (fs.storage_servers[fs.storage_servers[ssid].backuped_up_in[0]].working == 1)
        {
            return (get_socket2(ip, fs.storage_servers[fs.storage_servers[ssid].backuped_up_in[0]].port_of_ss, fs.storage_servers[ssid].backuped_up_in[0]));
        }
        else if(fs.storage_servers[fs.storage_servers[ssid].backuped_up_in[1]].working == 1)
        {
            return (get_socket2(ip, fs.storage_servers[fs.storage_servers[ssid].backuped_up_in[1]].port_of_ss, fs.storage_servers[ssid].backuped_up_in[1]));
        }
        else
        {
            printf("No backup servers are working\n");
            return -1;
        }
    }
    int storage_server_socket = socket(PF_INET, SOCK_STREAM, PF_UNSPEC);
    if (storage_server_socket < 0)
    {
        perror("Error: Failed to create socket");
        exit(1);
    }

    struct sockaddr_in storage_server_addr;
    char *ip_address = "127.0.0.1";

    // Initialize storage_server_addr with zeros
    memset(&storage_server_addr, 0, sizeof(storage_server_addr));

    storage_server_addr.sin_family = AF_INET;
    storage_server_addr.sin_port = htons(port_num);

    // Convert IP address string to binary form
    if (inet_pton(AF_INET, ip_address, &storage_server_addr.sin_addr) <= 0)
    {
        // perror("inet_pton");
        printf("inet_pton error occured trying for backup servers\n");
        // exit(1);
        fs.storage_servers[ssid].working = 0;
        return(get_socket2(ip_address, fs.storage_servers[ssid].port_of_ss, ssid));
    }
    if (connect(storage_server_socket, (struct sockaddr *)&storage_server_addr, sizeof(storage_server_addr)) < 0)
    {
        printf("connect error occured trying for backup servers\n");
        // exit(1);
        fs.storage_servers[ssid].working = 0;
        return(get_socket2(ip_address, fs.storage_servers[ssid].port_of_ss, ssid));
    }
    return storage_server_socket;
}

void backup_things(int ssid, char *message)
{
    // create a socket and connect the client with the storage server
    // get the backup ssid's from the -> backened_up_in array and send the same messages to it after creating a socket
    printf("In backup_things\n");
    char *ip_address = "127.0.0.1";
    int ssid_1 = fs.storage_servers[ssid].backuped_up_in[0];
    char *message_for_1 = add_ss_to_message(ssid_1, -1, message);
    int backup_socket_1 = get_socket2(ip_address, fs.storage_servers[ssid_1].port_of_ss, ssid_1);
    send_message(&backup_socket_1, message_for_1);
    struct recv_msg_t msg = recv_message_server(&backup_socket_1);
    printf("Received message from the storage server: %s\n", msg.message);
    close(backup_socket_1);
    int ssid_2 = fs.storage_servers[ssid].backuped_up_in[1];
    char *message_for_2 = add_ss_to_message(ssid_2, -1, message);
    int backup_socket_2 = get_socket2(ip_address, fs.storage_servers[ssid_2].port_of_ss, ssid_2);
    send_message(&backup_socket_2, message_for_2);
    msg = recv_message_server(&backup_socket_2);
    close(backup_socket_2);
    printf("Received message from the storage server: %s\n", msg.message);
}
// Function to search in LRU_cache, and efficient search using tries.
int all_search(char *file)
{
    int storage_server_index = -1;
    // search in LRU_cache
    storage_server_index = LRU_search(cacheQueue, file);
    if (storage_server_index != -1)
    {
        return storage_server_index;
    }
    // search in tries
    storage_server_index = Efficient_search(root, file);
    if (storage_server_index != -1)
    {
        return storage_server_index;
    }
    return -1;
}

void copy_execution(int *socket_fd, struct nfs_network *fs, struct Command *command, char *message, int storage_server_index_read, int storage_server_index_write)
{
    char *ip_address = "127.0.0.1";
    int storage_server_index_read_socket = get_socket2(ip_address, fs->storage_servers[storage_server_index_read].port_of_ss, storage_server_index_read);
    char *ss_added_message = add_ss_to_message(storage_server_index_read, -1, message);
    char read_message[50];
    strcpy(read_message, "cat ");
    char *second_token = strtok(ss_added_message, " ");
    second_token = strtok(NULL, " ");
    if (second_token != NULL)
    {
        strcat(read_message, second_token);
    }
    printf("read_message: %s\n", read_message);
    // add \r\n to the read_message
    strcat(read_message, "\r\n");
    send_message(&storage_server_index_read_socket, read_message);
    struct recv_msg_t msg = recv_message_server(&storage_server_index_read_socket);
    printf("Received message from the storage server: %s\n", msg.message);
    char *last_token = NULL;
    char *token = strtok(msg.message, " \r\n");
    close(storage_server_index_read_socket);
    while (token != NULL)
    {
        last_token = token;
        token = strtok(NULL, " \n\r");
    }
    if (last_token == NULL)
    {
        send_message(socket_fd, "File doesn't have any content to copy");
    }
    else
    {
        // all_search for file in command->data
        int storage_server_index_write_socket = get_socket2(ip_address, fs->storage_servers[storage_server_index_write].port_of_ss, storage_server_index_write);
        char write_message[50];
        strcpy(write_message, "write ");
        strcat(write_message, command->data);
        strcat(write_message, " ");
        strcat(write_message, last_token);
        strcat(write_message, "\r\n");
        printf("write_message: %s\n", write_message);
        char *ss_added_message = add_ss_to_message(storage_server_index_write, -1, write_message);
        send_message(&storage_server_index_write_socket, ss_added_message);
        struct recv_msg_t msg = recv_message_server(&storage_server_index_write_socket);
        send_message(socket_fd, msg.message);
        close(storage_server_index_write_socket);
        backup_things(storage_server_index_write, write_message);
    }
}

void search_and_send_to_storage_server(int *socket_fd, struct nfs_network *fs, struct Command *command, char *message)
{
    // Iterate over the file mappings to find the storage server for the given file
    int storage_server_index = -1;
    if (command->type != mkdir_cmd && command->type != mkfile_cmd && command->type != ls_cmd)
    {
        storage_server_index = all_search(command->file);

        if (storage_server_index == -1)
        {
            printf("File not found in any storage server.\n");
            return;
        }

        // Get the socket for the found storage server
        printf("Found storage server index: %d\n", storage_server_index);
    }
    else if (command->type == mkdir_cmd || command->type == mkfile_cmd)
    {
        printf("We are NOT entering search\n");
        int ssid;
        int curr_storage = 1e9;
        // for (int i = 0; i < MAX_STORAGE_SERVERS; i++)
        // {
        //     if (fs->storage_servers[i].working)
        //     {
        //         if (curr_storage > fs->storage_servers[i].current_storage_capacity)
        //         {
        //             curr_storage = fs->storage_servers[i].current_storage_capacity;
        //             ssid = i;
        //         }
        //     }
        // } uncomment this later.

        int random = rand() % 1;
        random = 0; // for testing purposes
        printf("random: %d\n", random);
        if (random == 0)
        {
            ssid = 0;
        }
        else
        {
            ssid = 1;
        }
        uint32_t ssid_port_number = fs->storage_servers[ssid].port_of_ss;
        uint32_t ssid_ip_address = fs->storage_servers[ssid].ip_of_ss;

        char *ip_address = "127.0.0.1";
        printf("ssid_port_number to which NS is connecting: %d\n", ssid_port_number);
        printf("ssid_ip_address to which NS is connecting: %d\n", ssid_ip_address);

        // create a socket and connect the client with the storage server
        int storage_server_socket = get_socket2(ip_address, ssid_port_number, ssid);
        // print message
        printf("Sending %s to the storage server\n", message);
        char *ss_added_message = add_ss_to_message(ssid, -1, message);
        send_message(&storage_server_socket, ss_added_message);
        printf("Sent message from the SS to the NS");
        // receive the message from the storage server
        struct recv_msg_t msg = recv_message_server(&storage_server_socket);
        printf("Received message from the storage server: %s\n", msg.message);
        // get the first three characters of msg.message
        char code[3];
        for (int i = 0; i < 3; i++)
        {
            code[i] = msg.message[i];
        }
        if (strcmp(code, "200") == 0)
        {
            // add to the file mappings and num files
            printf("Adding to the file mappings\n");
            fs->file_mappings[fs->storage_servers->num_of_files].storage_server_index = ssid;
            strcpy(fs->file_mappings[fs->storage_servers->num_of_files].file_name, command->file);
            fs->storage_servers->num_of_files++;
            insert(root, command->file, ssid);
            // add to the LRU cache
            enqueue(cacheQueue, command->file, ssid);
        }
        send_message(socket_fd, msg.message);
        close(storage_server_socket);
        backup_things(ssid, message);
        // backup servers also send message
    }
    if (command->type == cat_cmd || command->type == write_cmd || command->type == stat_cmd)
    {
        char* ip = "127.0.0.1";
        uint32_t storage_server_ip_address = fs->storage_servers[storage_server_index].ip_of_ss;
        char storage_server_ip_string[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &storage_server_ip_address, storage_server_ip_string, INET_ADDRSTRLEN);
        uint16_t storage_server_port_num = fs->storage_servers[storage_server_index].port_of_ss;
        char storage_server_port_string[6]; // Port number can be up to 5 digits + null terminator
        sprintf(storage_server_port_string, "%u", storage_server_port_num);
        enqueue(cacheQueue, command->file, storage_server_index);
        int storage_server_id = fs->storage_servers[storage_server_index].id;
        // send storage server id only if it is working else go for backups
        if(fs->storage_servers[storage_server_index].working == 0 || check_connection(ip, fs->storage_servers[storage_server_index].port_of_ss) == -1)
        {
            printf("Storage server is not working going for backups\n");
            fs->storage_servers[storage_server_index].working = 0;
            if(fs->storage_servers[fs->storage_servers[storage_server_index].backuped_up_in[0]].working == 1 && check_connection(ip, fs->storage_servers[fs->storage_servers[storage_server_index].backuped_up_in[0]].port_of_ss) != -1)
            {
                printf("Backup server 1 is working\n");
                storage_server_id = fs->storage_servers[storage_server_index].backuped_up_in[0];
                storage_server_port_num = fs->storage_servers[storage_server_index].port_of_ss;
            }
            else if(fs->storage_servers[fs->storage_servers[storage_server_index].backuped_up_in[1]].working == 1 && check_connection(ip, fs->storage_servers[fs->storage_servers[storage_server_index].backuped_up_in[1]].port_of_ss) != -1)
            {
                printf("Backup server 2 is working\n");
                storage_server_id = fs->storage_servers[storage_server_index].backuped_up_in[1];
                storage_server_port_num = fs->storage_servers[storage_server_index].port_of_ss;
            }
            else
            {
                printf("No backup servers are working\n");
                return;
            }
        }
        // convert storage_server_id to string with a int to string function
        char storage_server_id_string[10];
        sprintf(storage_server_id_string, "%d", storage_server_id);
        char message1[1024]; // Ensure this buffer is large enough
        sprintf(message1, "%s %s %s", storage_server_ip_string, storage_server_port_string, storage_server_id_string);
        char *storage_server_info = format_response("200 OK", message1);
        send_message(socket_fd, storage_server_info);
        if (command->type == write_cmd)
        {
            printf("%s\n for backup", command->data);
            backup_things(storage_server_index, message);
        }
    }
    else if (command->type == ls_cmd)
    {
        // char *ip_address =
        // send message as all files in the storage server seperated with /n you can get it from mappings
        char *ls_message = (char *)malloc(sizeof(char) * 1024 * 20);
        strcpy(ls_message, "");
        // print all files in file_mappings
        for (int i = 0; i < 10; i++)
        {
            strcat(ls_message, fs->file_mappings[i].file_name);
            printf("file name: %s\n", fs->file_mappings[i].file_name);
        }
        char *ls_response = format_response("200 OK", ls_message);
        send_message(socket_fd, ls_response);
    }
    else if (command->type == cp_cmd)
    {
        // assuming command->data is the file name
        // storaage_server_index read, command.data write with content returned by reading command->file
        int storage_server_read_index = all_search(command->file);
        if (storage_server_read_index == -1)
        {
            printf("File not found in any storage server.\n");
            return;
        }
        int storage_server_write_index = all_search(command->data);
        if (storage_server_write_index == -1)
        {
            printf("File not found in any storage server.\n");
            return;
        }
        copy_execution(socket_fd, fs, command, message, storage_server_read_index, storage_server_write_index);
    }
    else if (command->type == rmdir_cmd || command->type == rm_cmd)
    {
        char *ip_address = "127.0.0.1";
        int storage_server_socket = get_socket2(ip_address, fs->storage_servers[storage_server_index].port_of_ss, storage_server_index);

        char *ss_added_message = add_ss_to_message(storage_server_index, -1, message);
        send_message(&storage_server_socket, ss_added_message);
        struct recv_msg_t msg = recv_message_server(&storage_server_socket);
        send_message(socket_fd, msg.message);

        char code[3];
        for (int i = 0; i < 3; i++)
        {
            code[i] = msg.message[i];
        }
        if (strcmp(code, "200") == 0)
        {
            for (int i = 0; i < MAX_FILES; i++)
            {
                if (strcmp(fs->file_mappings[i].file_name, command->file) == 0)
                {
                    strcpy(fs->file_mappings[i].file_name, "");
                    fs->file_mappings[i].storage_server_index = -1;
                    fs->storage_servers[storage_server_index].num_of_files--;
                    removeFileName(root, command->file);
                    LRU_set_to_neg(cacheQueue, command->file);
                    break;
                }
            }
        }
        close(storage_server_socket);
        backup_things(storage_server_index, message);
    }
    // else if(command->type == stat_cmd)
    // {
    //     printf("Entered stat command\n");
    //     char* ip_address =  "127.0.0.1";
    //     int storage_server_socket = get_socket2(ip_address, fs->storage_servers[storage_server_index].port_of_ss);
    //     char* ss_added_message = add_ss_to_message(storage_server_index, -1, message);
    //     send_message(&storage_server_socket, ss_added_message);
    //     struct recv_msg_t msg = recv_message_server(&storage_server_socket);
    //     send_message(socket_fd, msg.message);
    //     close(storage_server_socket);

    // }
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
    root = createNode();
    cacheQueue = createLRUCacheQueue(5);
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
