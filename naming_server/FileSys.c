// #include "FileSys.h"
// #include "BasicFileSys.h"
// #include "Blocks.h"



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../common/structures.h"
#include "../common/helper.h"
#include <sys/stat.h>

// Forward declare functions
// void validate_before_new_entry(char* dir, char* name);
// extern char* format_response(char* code, char* message);
// extern void send_message(int sock_fd, char* message);


#define MAX_STORAGE_SERVERS 3
#define MAX_FILENAME_LENGTH 256

void init_storage_servers(struct storage_server servers[3]) {
    
    for (int i = 0; i < 3; i++) {
        // servers[i].ip_of_ss = 0x7F000001; // 127.0.0.1 as an example
        inet_pton(AF_INET, "127.0.0.1", &servers[i].ip_of_ss); // Convert string IP to numeric
        servers[i].port_of_ss = 10000 + i; // Ports 50000, 10001, 10002
        servers[i].last_backup_itself_at = 0;
        servers[i].last_backup_of_other_at = 0;
        servers[i].working = 1;
        servers[i].backup_memory_consumed[0] = 0; // for 0th server it is 1. // for 1st server it is 0. // for 2nd server it is 0.
        servers[i].backup_memory_consumed[1] = 0; // for 0th server it is 2. // for 1st server it is 2. // for 2nd server it is 1.
        //  x%3 == 0. then  +1,+2; x%3 == 1. then  -1,+1;  x%3 == 2. then  -2,-1;
        servers[i].backup_of[0] = 1; // => need to frame up in a generalized way.
        servers[i].backup_of[1] = 2; // => need to frame up in a generalized way.
        // servers[i].backuped_up_in[0] = 0; // => need to frame up in a generalized way.
        // servers[i].backuped_up_in[1] = 1; // => need to frame up in a generalized way.
        if(i % 3 == 0) {
            servers[i].backuped_up_in[0] = i + 1;
            servers[i].backuped_up_in[1] = i + 2;
        } else if(i % 3 == 1) {
            servers[i].backuped_up_in[0] = i - 1;
            servers[i].backuped_up_in[1] = i + 1;
        } else {
            servers[i].backuped_up_in[0] = i - 2;
            servers[i].backuped_up_in[1] = i - 1;
        }
        servers[i].current_storage_capacity = 0;
        servers[i].num_of_files = 0;
        servers[i].id = i;
        // create a folder for each storage server with name ss_<id>
        char dir[10];
        snprintf(dir, sizeof(dir), "ss_%d", i);
        mkdir(dir, 0777);
    }
}

#include <netinet/in.h> // For struct sockaddr_in
#include <string.h>     // For memset

void create_server_sockets(struct nfs_network *network) {
    for (int i = 0; i < MAX_STORAGE_SERVERS; i++) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = (network->storage_servers[i].ip_of_ss); // Use the integer IP address
        server_addr.sin_port = htons(network->storage_servers[i].port_of_ss);
        // printf("ip_of_ss: %d\n", network->storage_servers[i].ip_of_ss);
        // Debug print
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &network->storage_servers[i].ip_of_ss, ip_str, INET_ADDRSTRLEN);
        printf("Attempting to connect to Server IP: %s, Port: %d\n", ip_str, (network->storage_servers[i].port_of_ss));

        // Connect to the storage server
        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Failed to connect to storage server");
            close(sock);
            continue; // Optionally continue to try the next server or handle this error differently
        }

        network->server_sockets[i] = sock;
        printf("Connected to storage server %d\n", i);
    }
}




// mounts the file system
void fs_mount(struct nfs_network* network)
{
    // Assuming we dont have any servers before hand, like before we start the
    // server, lets create 3 storage servers.
    // We will use the same port for all 3 servers, but different ports can be
    // used as well.
    // initalize an array of storage servers
    // initalize the storage servers
    printf("Calling init_storage servers\n");
    struct storage_server storage_servers[3];
    init_storage_servers(storage_servers);
    // storage_servers[0].files[0] = "file1";
    // struct nfs_network network;
    memcpy(network->storage_servers, storage_servers, sizeof(storage_servers));
    // create_server_sockets(network);
    // strncpy(network->file_mappings[0].file_name, "file1", MAX_FILENAME_LENGTH - 1);
    network->file_mappings[0].file_name[MAX_FILENAME_LENGTH - 1] = '\0'; // Ensure null-termination

    network->file_mappings[0].storage_server_index = 0;
}