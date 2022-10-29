#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_defs.h"

#define buffer_size 500

int curr_states = 0;

struct state{
    int client_fd;
    int ftp_port;
    int client_ftp_connection;
    char ip_addr[IP_SIZE];
} SERVER_STATE[MAX_CONNECTIONS];

int get_state_ind(int client_fd){
    for (int i=0;i<MAX_CONNECTIONS;i++){
        if (SERVER_STATE[i].client_fd==client_fd){
            return i;
        }
    }
    return -1;
}

