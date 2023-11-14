// #include "FileSys.h"
#include "BasicFileSys.h"
#include "Blocks.h"

#include "nfs_helper.h"


#include "structures.h"

// Forward declare functions
void validate_before_new_entry(char* dir, char* name);
extern char* format_response(char* code, char* message);
extern void send_message(int sock_fd, char* message);

// mounts the file system
void mount_main_nfs(char* server_ip, int server_port, char* mount_point) {
    
    char* curr_dir = "/nfs_server";
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        printf("Error creating socket\n");
        return;
    }
    struct sockaddr_in server_addr = get_server_addr(server_ip, server_port);
    if (connect(sock_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        printf("Error connecting to server\n");
        return;
    }
    char* message = format_response("mount", mount_point);
    send_message(sock_fd, message);
    free(message);
    set_working_dir(curr_dir);

}