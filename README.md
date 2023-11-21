# Network File System

The NFS project comprises three fundamental components: Clients, the Naming Server, and Storage Servers. Clients serve as the primary interface, enabling file operations such as reading, writing, deletion, and more within the NFS. The Naming Server acts as the pivotal intermediary, orchestrating communication between clients and storage servers by providing crucial file location details. Storage Servers form the backbone, responsible for securely storing and retrieving files across the network. 

# COMMANDS

## Server Setup

1. Compile the server code using `./nfs 8000`.
2. The server will be initialized and listen on port `8000`.

## Client Connection

1. Execute `./client_exe localhost:8000` to connect a client to the server.
2. Replace `localhost:8000` with the appropriate server address and port if needed.

## Storage Server set-up

1. Execute `./ss 50000 0` to connect a client to the server.



### List of commands which should NOT search.

1. `mkdir <folder_name>`
2. `mkfile <file_name>`


### List of commands which should search.
1. `cat <file_name>`
2. `write <file_name> content`
3. `stat <file_name>`
4. `rm <file_name>`
5. `rmdir <folder_name>`
6. `copy <file_name> <new_file_name>`
7. `ls`

## Assumptions

1. On using `write` we can only send one NO-SPACED entity as the content of the file.
2. `stat` displays the file size in bytes and permission only.
3. Tries was used to implement the search functionality.
4. In detail logging was done in the all the three channels on the terminal, because there was no mention of a file for logging.
5. On doing `mkfile` on a folder which does not exist, the folder is created and the file is created inside it.
6. On doing `mkdir` on a folder which does not exist, the folder is created.
6. The STOP packet which we understood was the usage of `\r\n` in the end of the command.

