// extern the fs_cat and fs_create funtions implemented in functionalities.c

// Path: functionalities.c

#ifndef FUNCTIONALITIES_H
#define FUNCTIONALITIES_H

#include <stdio.h>  // For file operations

// Forward declarations of the functions
void fs_create(int *socket_fd, const char *file_name);
void fs_cat(int *socket_fd, const char *file_name);
void fs_mkdir(int *socket_fd, const char *dir_name);
void fs_mkfile(int *socket_fd, const char *file_name);
void fs_write(int *socket_fd, const char *file_name, const char *data);
void fs_rm(int *socket_fd, const char *file_name);
void fs_rmdir(int *socket_fd, const char *dir_name);
void fs_stat(int *socket_fd, const char *file_name);
#endif // FUNCTIONALITIES_H
