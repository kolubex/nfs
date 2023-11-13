### Changes I made:
1. Created shell.h as a clone for Shell.h
2. Created a client.c as a clone for Client.cpp
3. Created a shell.c as a clone for Shell.cpp
    - There are something like cs_sock, if it poses an error replace it with shell->cs_sock
    - Removed network_command, don't forget to add in shell.c
4. Created helper.h as a clone for Helper.h

