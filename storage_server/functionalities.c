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
#include <dirent.h>
#include <errno.h>
#define PATH_MAX 4096
char *format_response(const char *code, const char *message);

void send_message(int *sock_fd, char *message);
void response_ok(char *message, int *fs_sock)
{
    // Send the data (response) thought the socket.
    char *formatted_message = format_response("200 OK", message);
    printf("formatted_message: %s\n", formatted_message);
    send_message(fs_sock, formatted_message);
}

void fs_create(int *socket_fd, const char *file_name)
{
    FILE *file = fopen(file_name, "w"); // Open the file for writing
    if (file == NULL)
    {
        char *error_message = "Error: Unable to create file";
        send_message(socket_fd, error_message);
        return;
    }

    fclose(file); // Close the file after creating it
    char *success_message = "File created successfully";
    response_ok(success_message, socket_fd);
}

void fs_cat(int *socket_fd, const char *file_name)
{
    printf("fs_cat\n");
    FILE *file = fopen(file_name, "r"); // Open the file for reading
    if (file == NULL)
    {
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
    if (file_content == NULL)
    {
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

// void fs_mkdir(int *socket_fd, const char *dir_name)
// {
//     printf("fs_mkdir\n");
//     int status = mkdir(dir_name, 0777); // Create a directory with read, write, execute permissions for everyone
//     if (status < 0)
//     {
//         char *error_message = "Error: Unable to create directory";
//         send_message(socket_fd, error_message);
//         return;
//     }

//     char *success_message = "Directory created successfully";
//     response_ok(success_message, socket_fd);
// }


void fs_mkdir(int *socket_fd, const char *dir_name) {
    printf("fs_mkdir\n");

    char *dir_copy = strdup(dir_name); // Create a copy to avoid modifying the original string
    char *token = strtok(dir_copy, "/");
    char path[256] = ""; // Adjust size as needed

    while (token != NULL) {
        strcat(path, token);
        strcat(path, "/");

        if (mkdir(path, 0777) != 0 && errno != EEXIST) {
            char *error_message = "Error: Unable to create directory";
            send_message(socket_fd, error_message);
            free(dir_copy);
            return;
        }

        token = strtok(NULL, "/");
    }

    free(dir_copy);

    char *success_message = "Directory created successfully";
    response_ok(success_message, socket_fd);
}


// void fs_mkfile(int *socket_fd, const char *file_name)
// {
//     printf("fs_mkfile\n");
//     FILE *file = fopen(file_name, "w"); // Open the file for writing
//     if (file == NULL)
//     {
//         char *error_message = "Error: Unable to create file";
//         send_message(socket_fd, error_message);
//         return;
//     }

//     fclose(file); // Close the file after creating it
//     char *success_message = "File created successfully";
//     response_ok(success_message, socket_fd);
// }


void fs_write(int *socket_fd, const char *file_name, const char *data)
{
    printf("fs_write\n");
    FILE *file = fopen(file_name, "w"); // Open the file for writing
    if (file == NULL)
    {
        char *error_message = "Error: Unable to open file";
        send_message(socket_fd, error_message);
        return;
    }

    // Write the data to the file
    fprintf(file, "%s", data);
    fclose(file);

    char *success_message = "Data written successfully";
    response_ok(success_message, socket_fd);
}
void create_parent_directories(const char *file_name) {
    char *dir_copy = strdup(file_name);
    char *last_slash = strrchr(dir_copy, '/'); // Find the last occurrence of '/'
    if (last_slash == NULL) {
        free(dir_copy);
        return; // No directories to create
    }

    *last_slash = '\0'; // Truncate the string at the last '/'

    char *token = strtok(dir_copy, "/");
    char path[256] = "";

    while (token != NULL) {
        strcat(path, token);
        strcat(path, "/");

        if (mkdir(path, 0777) != 0 && errno != EEXIST) {
            free(dir_copy);
            return;
        }

        token = strtok(NULL, "/");
    }

    free(dir_copy);
}


void fs_mkfile(int *socket_fd, const char *file_name) {
    printf("fs_mkfile\n");

    create_parent_directories(file_name);

    FILE *file = fopen(file_name, "w");
    if (file == NULL) {
        char *error_message = "Error: Unable to create file";
        send_message(socket_fd, error_message);
        return;
    }

    fclose(file);

    char *success_message = "File created successfully";
    response_ok(success_message, socket_fd);
}


void fs_rm(int *socket_fd, const char *file_name)
{
    printf("fs_rm\n");
    int status = remove(file_name);
    if (status < 0)
    {
        char *error_message = "Error: Unable to remove file";
        send_message(socket_fd, error_message);
        return;
    }

    char *success_message = "File removed successfully";
    response_ok(success_message, socket_fd);
}

void fs_rmdir(int *socket_fd, const char *path) {
    struct dirent *entry;
    DIR *dir = opendir(path);

    if (dir == NULL) {
        perror("Error opening directory");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

            struct stat statbuf;
            if (stat(full_path, &statbuf) != 0) {
                perror("Error getting status");
                continue;
            }

            if (S_ISDIR(statbuf.st_mode)) {
                fs_rmdir(socket_fd, full_path);
            } else {
                if (remove(full_path) != 0) {
                    char error_message[100];
                    snprintf(error_message, sizeof(error_message), "Error removing file: %.70s", full_path);
                    send_message(socket_fd, error_message);
                }
            }
        }
    }

    closedir(dir);

    if (rmdir(path) != 0) {
        char error_message[100];
        snprintf(error_message, sizeof(error_message), "Error removing directory: %.70s", path);
        send_message(socket_fd, error_message);
    } else {
        char success_message[100];
        snprintf(success_message, sizeof(success_message), "Directory removed successfully: %.67s", path);
        response_ok(success_message, socket_fd);
    }
}