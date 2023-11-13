#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shell.h"

#define MAX_NAME_LENGTH 100

struct Command
{
    char name[MAX_NAME_LENGTH];
    char file_name[MAX_NAME_LENGTH];
    char append_data[MAX_NAME_LENGTH];
};

struct Shell
{
    int cs_sock;
    int is_mounted;
};


int main(int argc, char **argv)
{
    struct Shell shell;
    struct Command myCommand;

    if (argc == 2)
    {
        mountNFS(&shell, argv[1]);
        run(&shell);
    }
    else if (argc == 4 && strcmp(argv[1], "-s") == 0)
    {
        mountNFS(&shell, argv[3]);
        run_script(&shell, argv[2]);
    }
    else
    {
        fprintf(stderr, "Invalid command line\n");
        fprintf(stderr, "Usage (one of the following):\n");
        fprintf(stderr, "./nfsclient server:port\n");
        fprintf(stderr, "./nfsclient -s <script-name> server:port\n");
        return 1; // Indicate an error
    }

    return 0;
}
