#ifndef SHELL_H
#define SHELL_H

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include "../common/structures.h"
#include "../common/helper.h"

#define MAX_NAME_LENGTH 100

struct Command
{
  char name[MAX_NAME_LENGTH];
  char file_name[MAX_NAME_LENGTH];
  char write_data[MAX_NAME_LENGTH];
};

struct Shell
{
  int cs_sock;
  int is_mounted;
};

void mountNFS(struct Shell *shell, char *fs_loc);

void unmountNFS(struct Shell *shell);

void run(struct Shell *shell, char *port);

void run_script(struct Shell *shell, char *file_name);

int execute_command(struct Shell *shell, char *command_str);

struct Command parse_command(char *command_str);

void mkdir_rpc(struct Shell *shell, char *dname);

void cd_rpc(struct Shell *shell, char *dname);

void home_rpc(struct Shell *shell);

void rmdir_rpc(struct Shell *shell, char *dname);

void ls_rpc(struct Shell *shell);

void create_rpc(struct Shell *shell, char *fname);

void write_rpc(struct Shell *shell, char *fname, char *data);

void cat_rpc(struct Shell *shell, char *fname);

void head_rpc(struct Shell *shell,char *fname, int n);

void rm_rpc(struct Shell *shell, char *fname);

void stat_rpc(struct Shell *shell, char *fname);

void network_command(struct Shell *shell, const char *message, int can_be_empty, int receives_ss_ip);


#endif