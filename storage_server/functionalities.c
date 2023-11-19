#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "../common/structures.h"
#include "../common/helper.h"
#include <sys/types.h>
#include <sys/stat.h>


char *format_response(const char *code, const char *message);

void send_message(int* sock_fd, char *message);
void response_ok(char* message, int *fs_sock)
{
  // Send the data (response) thought the socket.
  char* formatted_message = format_response("200 OK", message);
  printf("formatted_message: %s\n", formatted_message);
  send_message(fs_sock, formatted_message);
}



void fs_create(int *socket_fd, const char *file_name) {
    FILE *file = fopen(file_name, "w"); // Open the file for writing
    if (file == NULL) {
        char *error_message = "Error: Unable to create file";
        send_message(socket_fd, error_message);
        return;
    }

    fclose(file); // Close the file after creating it
    char *success_message = "File created successfully";
    response_ok(success_message, socket_fd);
}

void fs_cat(int* socket_fd, const char *file_name) {
    printf("fs_cat\n");
    FILE *file = fopen(file_name, "r"); // Open the file for reading
    if (file == NULL) {
        char *error_message = "Error: Unable to open file";
        send_message(socket_fd, error_message);
        return;
    }

    // Seek to the end of the file to determine its size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Allocate memory for the entire content
    char *file_content = (char *)malloc(file_size + 1);
    if (file_content == NULL) {
        char *error_message = "Error: Unable to allocate memory for file content";
        send_message(socket_fd, error_message);
        fclose(file);
        return;
    }

    // Read the entire file into memory
    fread(file_content, 1, file_size, file);
    file_content[file_size] = '\0'; // Null-terminate the string

    // Send the file content as a response
    response_ok(file_content, socket_fd);

    // Clean up
    free(file_content);
    fclose(file);
}

void fs_mkdir(int *socket_fd, const char *dir_name) {
    printf("fs_mkdir\n");
    int status = mkdir(dir_name, 0777); // Create a directory with read, write, execute permissions for everyone
    if (status < 0) {
        char *error_message = "Error: Unable to create directory";
        send_message(socket_fd, error_message);
        return;
    }

    char *success_message = "Directory created successfully";
    response_ok(success_message, socket_fd);
}