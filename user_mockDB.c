#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_defs.h"

struct USER{
    int id; //index in order of appearance
    int auth; //0 for not authenticated, 1 otherwise
    char username[USERNAME_SIZE];
    char password[PASSWORD_SIZE];
} CURRENT_USERS[MAX_USERS];

int read_db(){
    FILE *fptr = fopen(USERS_DB_NAME, "r");;
    char *buff = NULL;
    size_t len = 0;
    size_t num_chars;
    int val_type = 0; //0 for username, 1 for password
    int user_ind = 0;

    if (fptr != NULL){
        while ((num_chars = getline(&buff, &len, fptr)) != -1){
            if (val_type==0){
                CURRENT_USERS[user_ind].id = user_ind;
                CURRENT_USERS[user_ind].auth = 0;
                strcpy(CURRENT_USERS[user_ind].username, buff);
            }
            else if (val_type==1){
                strcpy(CURRENT_USERS[user_ind].password, buff);
                user_ind++;
            }
            val_type=(val_type+1)%2;
        }
    }
    else{
        printf(DB_READ_FAIL);
    }

    return -1;

}

