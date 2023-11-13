// declare a struct for storage server
#include "../helper.h"

// define macro MAX_FILES
#define MAX_FILES 100000
#define MAX_FOLDERS 100000
#define MAX_STORAGE_CAPACITY 1000000000
struct storage_server
{
    int ip_of_ss;
    int port_of_ss;
    int id;
    int num_of_files;
    int num_of_folders;
    char* files[MAX_FILES];
    char* folders[MAX_FOLDERS];
    // int max_storage_capcicity = MAX_STORAGE_CAPACITY;
    int max_storage_capacity; // to be equated to MAX_STORAGE_CAPACITY in init_storage_server
    int current_storage_capacity;
    int backup_of;
    int backuped_up_in;
    int last_backup_itself_at; // to be equated to 0 in init_storage_server // should be in ms as time increases this should increase.
    int last_backup_of_other_at; // to be equated to 0 in init_storage_server // should be in ms as time increases this should increase.
    bool working; // initialised to true in init_storage_server and set to false in storage_server_failure
    int backup_memory_consumed; // is the current memory consumed by the backup of other storage server MAX is 5MB
    int own_storage_server_memory_consumed; // is the current memory consumed by the own storage server MAX is 5MB
};