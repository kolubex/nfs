#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h> // close()
#include "helper.h"
#include <arpa/inet.h>


#define MAX_BUFFER_SIZE 65535

char *format_response(const char *code, const char *message)
{
    const char *endline = "\r\n";

    // Calculate the length of the formatted response
    size_t response_length = strlen(code) + strlen(endline) + strlen("Length:") + strlen(endline) +
                             strlen(endline) + strlen(message) + 1; // 1 for null terminator

    // Allocate memory for the formatted response
    char *full_response = (char *)malloc(response_length);

    // Check if memory allocation was successful
    if (full_response == NULL)
    {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }

    // Construct the formatted response
    sprintf(full_response, "%s%sLength:%lu%s%s%s", code, endline, (unsigned long)strlen(message), endline, endline, message);

    return full_response;
}

void send_message(int *sock_fd, char *msg)
{
    char *msg_ptr = msg;
    // pad the msg to length 65535
    char message[MAX_BUFFER_SIZE];

    memset(message, 0, MAX_BUFFER_SIZE);
    memcpy(message, msg, strlen(msg));
    msg_ptr = message;
    int msg_size = strlen(message);
    int bytes_sent;

    while (msg_size > 0)
    {
        bytes_sent = send(*sock_fd, msg_ptr, msg_size, 0);
        if (bytes_sent < 0)
        {
            perror("Error sending message");
            return;
        }

        msg_ptr += bytes_sent;
        msg_size -= bytes_sent;
    }
    printf("Message sent\n");
}


struct recv_msg_t recv_message_client(int sock_fd)
{
    struct recv_msg_t msg;
    msg.quit = 0;
    msg.message = NULL;
    int current_length = 0;
    char temp_buff[65535];

    ssize_t size;
    printf("Waiting for message...\n");
    while (1)
    {
        size = recv(sock_fd, temp_buff, sizeof(temp_buff), 0);
        if (size > 0)
        {
            printf("Received %ld bytes\n", size);
            // print temp buff
            printf("temp_buff: %s\n", temp_buff);
            // New data. Copy into message string.
            char *new_message = (char *)realloc(msg.message, current_length + size + 1);
            if (new_message == NULL)
            {
                printf("This an error\n");
                perror("Memory allocation error\n");
                free(msg.message);
                msg.message = NULL;
                msg.quit = 1;
                return msg;
            }
            printf("Equating\n");
            msg.message = new_message;
            printf("Going to copy\n");
            memcpy(msg.message + current_length, temp_buff, size);
            current_length += size;
            msg.message[current_length] = '\0'; // Null-terminate
            printf("msg.message: %s\n", msg.message);
            // Check if we've received the full message

            // Start by identifying the length of the message body
            char *lenPos = strstr(msg.message, "Length:");
            if (lenPos == NULL)
            {
                continue;
            }
            char *lenEndPos = strstr(lenPos + 1, "\r\n");
            if (lenEndPos == NULL)
            {
                continue;
            }

            char lengthStr[10];
            memcpy(lengthStr, lenPos + 7, lenEndPos - lenPos - 7);
            lengthStr[lenEndPos - lenPos - 7] = '\0';

            int length = -1;
            if (sscanf(lengthStr, "%d", &length) != 1)
            {
                // Failed to parse length
                break;
            }

            // Move past the two sets of "\r\n" after the length
            char *bodyPos = strstr(lenEndPos + 1, "\r\n");
            if (bodyPos == NULL)
            {
                continue;
            }

            char *body = bodyPos + 2;
            if (strlen(body) < (size_t)length)
            {
                // We're still missing a portion of the message body
                continue;
            }

            // We've got the full message
            break;
        }
        else if (size == 0)
        {
            // The socket has closed on the other end
            msg.quit = 1;
            break;
        }
        else
        {
            // An error has occurred (size < 0)
            perror("Error while receiving message from socket");
            free(msg.message);
            msg.message = NULL;
            close(sock_fd);
            return msg;
        }
    }

    return msg;
}

struct recv_msg_t recv_message_server(int* sock_fd) {
    printf("recv_message_server\n");
    struct recv_msg_t msg;
    msg.quit = 0;
    msg.message = NULL;
    size_t current_length = 0;

    char temp_buff[65535];
    ssize_t size;
    printf("Entering while loop\n");
    while (1) {
        printf("recv_message_server while\n");
        size = recv(*sock_fd, temp_buff, sizeof(temp_buff), 0);
        printf("size: %ld\n", size);
        printf("temp_buff: %s\n", temp_buff);
        if (size > 0) {
            char *new_message = realloc(msg.message, current_length + size + 1);
            if (new_message == NULL) {
                perror("Memory allocation error");
                free(msg.message);
                msg.message = NULL;
                msg.quit = 1;
                return msg;
            }
            msg.message = new_message;
            memcpy(msg.message + current_length, temp_buff, size);
            current_length += size;
            msg.message[current_length] = '\0'; // Null-terminate
            printf("msg.message: %s\n", msg.message);

            // Check for end of message
            if (strstr(msg.message, "\r\n") != NULL) {
                break;
            }
        } else if (size == 0) {
            // Socket closed
            msg.quit = 1;
            break;
        } else {
            // Receive error
            perror("Error while receiving message from socket");
            free(msg.message);
            msg.message = NULL;
            close(*sock_fd);
            msg.quit = 1;
            return msg;
        }
        // print the message
        
    }
    printf("msg.message: %s\n", msg.message);
    return msg;
}

int get_socket(char *ip, int port_num)
{
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
        perror("inet_pton");
        exit(1);
    }
    if (connect(storage_server_socket, (struct sockaddr *)&storage_server_addr, sizeof(storage_server_addr)) < 0)
    {
        perror("Error: Failed to connect to storage server");
        exit(1);
    }
    return storage_server_socket;
}
