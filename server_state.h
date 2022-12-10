#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_defs.h"

int curr_states = 0;

//State object for the FTP server
struct state{
    int client_fd; //control socket
    int ftp_port; //port
    int client_ftp_connection; //data socket
    int login_ready; //1 if waiting for PASS, 0 otherwise
    char ip_addr[IP_SIZE]; //IP
    char directory[DIRECTORY_NAME_MAX]; //current directory for the client
    int authenticated; //1 if logged in, 0 otherwise
	char username[FILE_NAME_MAX];
} SERVER_STATE[MAX_CONNECTIONS];

//return state index based on control socket
int get_state_ind(int client_fd){
    for (int i=0;i<MAX_CONNECTIONS;i++){
        if (SERVER_STATE[i].client_fd==client_fd){
            return i;
        }
    }
    return -1;
}

//find free index to store state
int find_free_ind(){
	for (int i=0;i<MAX_CONNECTIONS;i++){
		if(SERVER_STATE[i].client_fd == -1){
			return i;
		}
	}
	return -1;
}

//initialize SERVER state
void init_state(){
	for (int i=0;i<MAX_CONNECTIONS;i++){
		SERVER_STATE[i].client_fd = -1;
		SERVER_STATE[i].ftp_port = -1;
		SERVER_STATE[i].client_ftp_connection = -1;
		SERVER_STATE[i].login_ready = -1;
		strcpy(SERVER_STATE[i].ip_addr,"");
		strcpy(SERVER_STATE[i].directory,"");
		strcpy(SERVER_STATE[i].username,"");
		SERVER_STATE[i].authenticated = -1;
	}
}
