// Implements the file system commands that are available to the shell.
#ifndef STRUCTURES_H
#define STRUCTURES_H
#include <stdbool.h>

#define MAX_STORAGE_SERVERS 3
#define MAX_FILENAME_LENGTH 256


// define macro MAX_FILES
#define MAX_FILES 100
#define MAX_DIRECTORIES 100
#define MAX_STORAGE_CAPACITY 1024
struct storage_server
{
    uint32_t ip_of_ss; // Use uint32_t for the IP address
    uint16_t port_of_ss; // Use uint16_t for the port
    int id;
    int num_of_files;
    char* files[MAX_FILES];
    // int max_storage_capcicity = MAX_STORAGE_CAPACITY;
    int max_storage_capacity; // to be equated to MAX_STORAGE_CAPACITY in init_storage_server
    int current_storage_capacity;
    int backup_of[2];
    int backuped_up_in[2];
    int last_backup_itself_at; // to be equated to 0 in init_storage_server // should be in ms as time increases this should increase.
    int last_backup_of_other_at; // to be equated to 0 in init_storage_server // should be in ms as time increases this should increase.
    int working; // initialised to true in init_storage_server and set to false in storage_server_failure
    int backup_memory_consumed[2]; // is the current memory consumed by the backup of other storage server MAX is 5MB
    int own_storage_server_memory_consumed; // is the current memory consumed by the own storage server MAX is 5MB
};


struct storage_server_mapping {
    char file_name[MAX_FILENAME_LENGTH];
    int storage_server_index;
};

struct nfs_network {
    struct storage_server storage_servers[MAX_STORAGE_SERVERS];
    struct storage_server_mapping file_mappings[MAX_FILES]; // Assuming MAX_FILES is defined
    int server_sockets[MAX_STORAGE_SERVERS]; // Sockets for each storage server
    int num_servers;
};


enum CommandType
{
    mkdir_cmd,
    mkfile_cmd,
    ls_cmd,
    cd_cmd,
    home_cmd,
    rmdir_cmd,
    create_cmd,
    write_cmd,
    stat_cmd,
    cat_cmd,
    cp_cmd,
    rm_cmd,
    quit_cmd,
    noop_cmd,
    invalid_cmd
};

// Command structure
struct Command
{
    enum CommandType type;
    char file[256]; // Fixed-size buffer for filename
    char data[256]; // Fixed-size buffer for data
};

#endif