#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include "../common/structures.h"
#include "../common/helper.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_MESSAGE_LENGTH 65535
#define MAX_CODE_LENGTH 100
#define MAX_LENGTH_LENGTH 100

static const char *PROMPT_STRING = "NFS> ";
const char *endline = "\r\n";

#define MAX_COMMAND_LENGTH 100

void parse_response_to_get_ss(char* response, char* formatted_message)
{
    // message is in format "ss_ip ss_port"
    // you have to send the send the same formatted message to the socket created with given info
    // print response
    printf("Received message: %s\n", response);
    char* ss_ip = strtok(response, " ");
    char* ss_port = strtok(NULL, " ");
    ss_ip = "localhost";
    ss_port = "50000";
    if (ss_ip == NULL || ss_port == NULL)
    {
        printf("Error: Invalid response from server\n");
        return;
    }
    // make a socket with given ip and port and send formatted message
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int ret;
    printf("Getting address info\n");
    printf("ss_ip: %s, ss_port: %s\n", ss_ip, ss_port);
    ret = getaddrinfo(ss_ip, ss_port, &hints, &res);
    if (ret != 0)
    {
        fprintf(stderr, "Error: Could not obtain address information for \"%s:%s\"\n", ss_ip, ss_port);
        fprintf(stderr, "\tgetaddrinfo returned with %d\n", ret);
        return;
    }
    // make ss_ip as localhost
    printf("Connecting to %s:%s\n", ss_ip, ss_port);
    int ss_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (ss_sock < 0)
    {
        fprintf(stderr, "Error: Failed to create a socket\n");
        fprintf(stderr, "\tsocket returned with %d\n", ss_sock);
        freeaddrinfo(res);
        return;
    }
    if (connect(ss_sock, res->ai_addr, res->ai_addrlen) < 0)
    {
        fprintf(stderr, "Error: Failed to connect with server (%s:%s).\n", ss_ip, ss_port);
        fprintf(stderr, "\tDouble check that the server is running.\n");
        close(ss_sock);
        freeaddrinfo(res);
        return;
    }
    freeaddrinfo(res);
    printf("Sending message: %s", formatted_message);
    send_message(ss_sock, formatted_message);
    struct recv_msg_t msg = recv_message_client(ss_sock);
    if (msg.quit)
    {
        printf("Server has closed the connection\n");
        exit(EXIT_SUCCESS);
    }
    char *response2 = msg.message;
    // Parse response (remove the protocol header)
    // The code, length, and body are separated by "\r\n"
    char *code, *length, *body;
    size_t lenPos = strcspn(response, "\r\n");
    code = strndup(response, lenPos);
    size_t bodyPos = strcspn(response + lenPos + 2, "\r\n") + lenPos + 2;
    length = strndup(response + lenPos + 2, bodyPos - (lenPos + 2));
    body = strdup(response + bodyPos + 4); // There should be two sets of "\r\n"

    char *output = body;

    // if write command then print the output as success else print the output as it is


}
void mountNFS(struct Shell *shell, char *fs_loc)
{
    printf("Mounting file system\n");
    printf("File system location: %s\n", fs_loc);
    char hostname[100];
    char port[100];

    // Parse fs_loc string
    int i = 0;
    while (fs_loc[i] != ':' && fs_loc[i] != '\0') {
        hostname[i] = fs_loc[i];
        i++;
    }
    hostname[i] = '\0';

    if (fs_loc[i] == ':') {
        i++;
        int j = 0;
        while (fs_loc[i] != '\0') {
            port[j] = fs_loc[i];
            i++;
            j++;
        }
        port[j] = '\0';
    } else {
        fprintf(stderr, "Error: Invalid file system location\n");
        return;
    }

    // Debug print
    printf("Server Name: %s  Port: %s\n", hostname, port);

    struct addrinfo hints = {0};
    struct addrinfo *addr = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    // print hostname port
    printf("Getting address info\n");
    printf("hostname: %s, port: %s\n", hostname, port);
    int ret;
    if ((ret = getaddrinfo(hostname, port, &hints, &addr)) != 0) {
        fprintf(stderr, "Error: Could not obtain address information for \"%s:%s\"\n", hostname, port);
        fprintf(stderr, "\tgetaddrinfo returned with %d\n", ret);
        return;
    }

    // Create socket to connect
    
    shell->cs_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (shell->cs_sock < 0) {
        fprintf(stderr, "Error: Failed to create a socket\n");
        fprintf(stderr, "\tsocket returned with %d\n", shell->cs_sock);
        freeaddrinfo(addr);
        return;
    }

    // Connect to server
    if (connect(shell->cs_sock, addr->ai_addr, addr->ai_addrlen) < 0) {
        fprintf(stderr, "Error: Failed to connect with server (%s:%s).\n", hostname, port);
        fprintf(stderr, "\tDouble check that the server is running.\n");
        close(shell->cs_sock);
        freeaddrinfo(addr);
        return;
    }

    // Free the linked list created by getaddrinfo
    freeaddrinfo(addr);

    // Set is_mounted to true
    // Assuming is_mounted is a global variable or a part of a structure
    // You may need to adjust this based on your actual implementation
    shell->is_mounted = 1;
    printf("Successfully mounted!\n");

    // Close the socket if needed (depends on your design)
    // close(cs_sock);
}

void unmountNFS(struct Shell *shell)
{
    // Implementation for unmountNFS
    if (shell->is_mounted)
    {
        close(shell->cs_sock);
        shell->is_mounted = 0;
    }
}

void mkdir_rpc(struct Shell *shell, char *dname) {
    // Calculate the length needed for the command
    int cmd_length = snprintf(NULL, 0, "mkdir %s%s", dname, endline);
    
    // Allocate memory for the command
    char *cmd = malloc(cmd_length + 1);  // +1 for the null terminator

    if (cmd == NULL) {
        perror("Memory allocation error");
        return;
    }

    // Construct the command
    snprintf(cmd, cmd_length + 1, "mkdir %s%s", dname, endline);

    // Call the network_command function
    network_command(shell, cmd, 0, 0);

    // Free the allocated memory
    free(cmd);
}


void cd_rpc(struct Shell *shell, char *dname)
{
    char cmd[MAX_NAME_LENGTH + 8]; // command + "cd " + endline
    snprintf(cmd, sizeof(cmd), "cd %s%s", dname, endline);
    network_command(shell, cmd, 0, 0);
}

void home_rpc(struct Shell *shell)
{
    char cmd[6]; // "home" + endline
    snprintf(cmd, sizeof(cmd), "home%s", endline);
    network_command(shell, cmd, 0, 0);
}

void rmdir_rpc(struct Shell *shell, char *dname)
{
    char cmd[MAX_NAME_LENGTH + 10]; // command + "rmdir " + endline
    snprintf(cmd, sizeof(cmd), "rmdir %s%s", dname, endline);
    network_command(shell, cmd, 0, 0);
}

void ls_rpc(struct Shell *shell)
{
    char cmd[5]; // "ls" + endline
    snprintf(cmd, sizeof(cmd), "ls%s", endline);
    network_command(shell, cmd, 0, 0);
}

void create_rpc(struct Shell *shell, char *fname )
{
    char cmd[MAX_NAME_LENGTH + 12]; // command + "create " + endline
    snprintf(cmd, sizeof(cmd), "create %s%s", fname, endline);
    network_command(shell, cmd, 0, 0);
}

void write_rpc(struct Shell *shell, char *fname, char *data)
{
    char cmd[MAX_NAME_LENGTH * 2 + 15]; // command + "write " + endline
    snprintf(cmd, sizeof(cmd), "write %s %s%s", fname, data, endline);
    network_command(shell, cmd, 0, 1);
}

void cat_rpc(struct Shell *shell, char *fname)
{
    char cmd[MAX_NAME_LENGTH + 8]; // command + "cat " + endline
    snprintf(cmd, sizeof(cmd), "cat %s%s", fname, endline);
    network_command(shell, cmd, 1, 1);
}

void head_rpc(struct Shell *shell, char *fname, int n)
{
    char cmd[MAX_NAME_LENGTH + 15]; // command + "head " + endline
    snprintf(cmd, sizeof(cmd), "head %s %d%s", fname, n, endline);
    network_command(shell, cmd, 1, 1);
}

void rm_rpc(struct Shell *shell, char *fname)
{
    char cmd[MAX_NAME_LENGTH + 10]; // command + "rm " + endline
    snprintf(cmd, sizeof(cmd), "rm %s%s", fname, endline);
    network_command(shell, cmd, 0, 0);
}

void stat_rpc(struct Shell *shell, char *fname)
{
    char cmd[MAX_NAME_LENGTH + 11]; // command + "stat " + endline
    snprintf(cmd, sizeof(cmd), "stat %s%s", fname, endline);
    network_command(shell, cmd, 1, 1);
}

void run(struct Shell *shell)
{
    if (!shell->is_mounted) {
        printf("File system is not mounted. Exiting.\n");
        return;
    }

    // Continue until the user quits
    int user_quit = 0;
    while (!user_quit) {
        // Print prompt and get command line
        char command_str[MAX_COMMAND_LENGTH];
        printf("%s", PROMPT_STRING);

        if (fgets(command_str, sizeof(command_str), stdin) == NULL) {
            fprintf(stderr, "Error reading input.\n");
            break;
        }

        // Remove newline character from the end of the input
        command_str[strcspn(command_str, "\n")] = '\0';

        // Execute the command
        user_quit = execute_command(shell, command_str);
    }

    // Unmount the file system
    unmountNFS(shell);
}

void run_script(struct Shell *shell, char *file_name)
{
   if (!shell->is_mounted) {
        printf("File system is not mounted. Exiting.\n");
        return;
    }

    // Open script file
    FILE *infile = fopen(file_name, "r");
    if (!infile) {
        fprintf(stderr, "Could not open script file: %s\n", file_name);
        return;
    }

    // Execute each line in the script
    int user_quit = 0;
    char command_str[MAX_COMMAND_LENGTH];

    while (fgets(command_str, sizeof(command_str), infile) != NULL && !user_quit) {
        // Remove newline character from the end of the input
        command_str[strcspn(command_str, "\n")] = '\0';

        printf("%s%s\n", PROMPT_STRING, command_str);
        user_quit = execute_command(shell, command_str);
    }

    // Clean up
    unmountNFS(shell);
    fclose(infile);
}

int execute_command(struct Shell *shell, char *command_str)
{
    struct Command command = parse_command(command_str);

    // look for the matching command
    if (strcmp(command.name, "") == 0) {
        return 0;
    } else if (strcmp(command.name, "mkdir") == 0) {
        mkdir_rpc(shell, command.file_name);
    } else if (strcmp(command.name, "cd") == 0) {
        cd_rpc(shell, command.file_name);
    } else if (strcmp(command.name, "home") == 0) {
        home_rpc(shell);
    } else if (strcmp(command.name, "rmdir") == 0) {
        rmdir_rpc(shell, command.file_name);
    } else if (strcmp(command.name, "ls") == 0) {
        ls_rpc(shell);
    } else if (strcmp(command.name, "create") == 0) {
        create_rpc(shell, command.file_name);
    } else if (strcmp(command.name, "write") == 0) {
        write_rpc(shell, command.file_name, command.write_data);
    } else if (strcmp(command.name, "cat") == 0) {
        cat_rpc(shell, command.file_name);
    } else if (strcmp(command.name, "head") == 0) {
        errno = 0;
        unsigned long n = strtoul(command.write_data, NULL, 0);
        if (errno == 0) {
            head_rpc(shell, command.file_name, n);
        } else {
            fprintf(stderr, "Invalid command line: %s is not a valid number of bytes\n", command.write_data);
            return 0;
        }
    } else if (strcmp(command.name, "rm") == 0) {
        rm_rpc(shell, command.file_name);
    } else if (strcmp(command.name, "stat") == 0) {
        stat_rpc(shell, command.file_name);
    } else if (strcmp(command.name, "quit") == 0) {
        return 1;
    }

    return 0;

}

// 
void network_command(struct Shell *shell, const char *message, int can_be_empty, int receives_ss_ip)
{
    // Format message for network transit
    char *formatted_message = malloc(strlen(message) + strlen(endline) + 1);
    if (formatted_message == NULL)
    {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }

    sprintf(formatted_message, "%s%s", message, endline);

    // Send command over the network (through the provided socket)
    printf("Sending message: %s", formatted_message);
    send_message(shell->cs_sock, formatted_message);

    struct recv_msg_t msg = recv_message_client(shell->cs_sock);
    if (msg.quit)
    {
        printf("Server has closed the connection\n");
        exit(EXIT_SUCCESS);
    }

    char *response = msg.message;

    // Parse response (remove the protocol header)
    // The code, length, and body are separated by "\r\n"
    char *code, *length, *body;
    size_t lenPos = strcspn(response, "\r\n");
    code = strndup(response, lenPos);
    size_t bodyPos = strcspn(response + lenPos + 2, "\r\n") + lenPos + 2;
    length = strndup(response + lenPos + 2, bodyPos - (lenPos + 2));
    body = strdup(response + bodyPos + 4); // There should be two sets of "\r\n"

    char *output = body;
    if (strlen(body) == 0 && (strcmp(code, "200 OK") != 0))
    {
        output = code;
    }
    else if (strlen(body) == 0 && (strcmp(code, "200 OK") == 0) && !can_be_empty)
    {
        // Some servers rely on the client to display "success" rather than sending
        // it through the socket. This handles that case
        output = "success";
    }
    if (receives_ss_ip)
    {
        printf("Received message: %s\n", output);
        printf("Parsing response to get ss_ip and ss_port\n");
        parse_response_to_get_ss(output, formatted_message);
    }
    else{
        printf("%s\n", output);
    }

    // Free allocated memory
    free(formatted_message);
    free(code);
    free(length);
    free(body);
}

struct Command parse_command(char *command_str)
{ 
    struct Command empty = {"", "", ""};

    // grab each of the tokens (if they exist)
    struct Command command = {"", "", ""};
    int num_tokens = sscanf(command_str, "%s %s %s", command.name, command.file_name, command.write_data);

    // Check for empty command line
    if (num_tokens == 0) {
        return empty;
    }

    // Check for invalid command lines
    if (strcmp(command.name, "ls") == 0 ||
        strcmp(command.name, "home") == 0 ||
        strcmp(command.name, "quit") == 0) {
        if (num_tokens != 1) {
            fprintf(stderr, "Invalid command line: %s has improper number of arguments\n", command.name);
            return empty;
        }
    } else if (strcmp(command.name, "mkdir") == 0 ||
               strcmp(command.name, "cd") == 0 ||
               strcmp(command.name, "rmdir") == 0 ||
               strcmp(command.name, "create") == 0 ||
               strcmp(command.name, "cat") == 0 ||
               strcmp(command.name, "rm") == 0 ||
               strcmp(command.name, "stat") == 0) {
        if (num_tokens != 2) {
            fprintf(stderr, "Invalid command line: %s has improper number of arguments\n", command.name);
            return empty;
        }
    } else if (strcmp(command.name, "write") == 0 || strcmp(command.name, "head") == 0) {
        if (num_tokens != 3) {
            fprintf(stderr, "Invalid command line: %s has improper number of arguments\n", command.name);
            return empty;
        }
    } else {
        fprintf(stderr, "Invalid command line: %s is not a command\n", command.name);
        return empty;
    }

    return command;
}
