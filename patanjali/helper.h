#ifndef FILESYS_H
#define FILESYS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h> // close()

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

void send_message(int sock_fd, const char *message)
{
    const char *msg_ptr = message;
    int msg_size = strlen(message);
    int bytes_sent;

    while (msg_size > 0)
    {
        bytes_sent = send(sock_fd, msg_ptr, msg_size, 0);
        if (bytes_sent < 0)
        {
            perror("Error sending message");
            return;
        }

        msg_ptr += bytes_sent;
        msg_size -= bytes_sent;
    }
}

struct recv_msg_t
{
    char *message; // Assuming maximum message size
    int quit;
};

struct recv_msg_t recv_message_client(int sock_fd)
{
    struct recv_msg_t msg;
    msg.quit = 0;
    msg.message = NULL;

    char temp_buff[65535];
    ssize_t size;

    while (1)
    {
        size = recv(sock_fd, temp_buff, sizeof(temp_buff), 0);
        if (size > 0)
        {
            // New data. Copy into message string.
            char *new_message = (char *)realloc(msg.message, strlen(msg.message) + size + 1);
            if (new_message == NULL)
            {
                perror("Memory allocation error");
                free(msg.message);
                msg.message = NULL;
                msg.quit = 1;
                return msg;
            }

            msg.message = new_message;
            memcpy(msg.message + strlen(msg.message), temp_buff, size);
            msg.message[strlen(msg.message) + size] = '\0'; // Null-terminate

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

struct recv_msg_t recv_message_server(int sock_fd)
{
    struct recv_msg_t msg;
    msg.quit = 0;
    msg.message = NULL;

    char temp_buff[65535];
    ssize_t size;

    while (1)
    {
        size = recv(sock_fd, temp_buff, sizeof(temp_buff), 0);
        if (size > 0)
        {
            // New data. Copy into message string.
            char *new_message = (char *)realloc(msg.message, strlen(msg.message) + size + 1);
            if (new_message == NULL)
            {
                perror("Memory allocation error");
                free(msg.message);
                msg.message = NULL;
                msg.quit = 1;
                return msg;
            }

            msg.message = new_message;
            memcpy(msg.message + strlen(msg.message), temp_buff, size);
            msg.message[strlen(msg.message) + size] = '\0'; // Null-terminate

            // Start by identifying the length of the message body
            char *endPos = strstr(msg.message, "\r\n");
            if (endPos == NULL)
            {
                // Still missing the full message
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


#endif
