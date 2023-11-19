// extern the fs_cat and fs_create funtions implemented in functionalities.c

// Path: functionalities.c

#ifndef FUNCTIONALITIES_H
#define FUNCTIONALITIES_H

#include <stdio.h>  // For file operations

// Forward declarations of the functions
void fs_create(int *socket_fd, const char *file_name);
void fs_cat(int *socket_fd, const char *file_name);
void fs_mkdir(int *socket_fd, const char *dir_name);

#endif // FUNCTIONALITIES_H
