#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_defs.h"

struct USER{
    int id; //index in order of appearance
    int auth; //0 for not authenticated, 1 otherwise
    char username[USERNAME_SIZE]; //username
    char password[PASSWORD_SIZE]; //password
} CURRENT_USERS[MAX_USERS];

int read_db(){
    FILE *fptr = fopen(USERS_DB_NAME, "r");;
    char *buff = NULL;
    size_t len = 0;
    size_t num_chars;
    int val_type = 0; //0 for username, 1 for password
    int user_ind = 0; 

    if (fptr != NULL){
        //read value from file
        while ((num_chars = getline(&buff, &len, fptr)) != -1){
            if (val_type==0){ // store username
                CURRENT_USERS[user_ind].id = -1;
                CURRENT_USERS[user_ind].auth = 0;
                strcpy(CURRENT_USERS[user_ind].username, buff);
            }
            else if (val_type==1){
                //store password and increment current users
                strcpy(CURRENT_USERS[user_ind].password, buff);
                user_ind++;
            }
            val_type=(val_type+1)%2;
        }
        //close file
        fclose(fptr);
        return 1;
    }

    return -1;

}

int init_users(){
    for (int i=0;i<MAX_USERS;i++){
        CURRENT_USERS[i].id = -1;
    }
    return 1;
}

