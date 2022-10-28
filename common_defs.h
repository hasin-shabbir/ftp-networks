#ifndef COMMON_DEFS
#define COMMON_DEFS

#define CONTROL_PORT 6000
#define DATA_PORT 10000
#define MESSAGE_BUFFER_SIZE 256
#define SERVER_IP "127.0.0.1"

#define USERNAME_SIZE 500
#define PASSWORD_SIZE 500
#define MAX_USERS 20
#define USERS_DB_NAME "users.txt"
#define DB_READ_FAIL "server could not read user database\n"

#define USER_SUCCESS "331 Username OK, need password."
#define USER_FAILURE "530 Not logged in."
#define PASSWORD_SUCCESS "230 User logged in, proceed."
#define PASSWORD_FAILURE "530 Not logged in."

#endif