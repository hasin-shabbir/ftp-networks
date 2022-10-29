#ifndef COMMON_DEFS
#define COMMON_DEFS

#define CONTROL_PORT 6000
#define DATA_PORT 10000
#define MESSAGE_BUFFER_SIZE 256
#define SERVER_IP "127.0.0.1"
#define IP_SIZE 20

#define USERNAME_SIZE 500
#define PASSWORD_SIZE 500
#define MAX_USERS 20
#define USERS_DB_NAME "users.txt"
#define DB_READ_FAIL "server could not read user database\n"
#define MAX_CONNECTIONS 20

#define USER_SUCCESS "331 Username OK, need password."
#define USER_FAILURE "530 Not logged in."
#define PASSWORD_SUCCESS "230 User logged in, proceed."
#define PASSWORD_FAILURE "530 Not logged in."
#define QUIT_MESSAGE "221 Service closing control connection."
#define PORT_OK "200 PORT command successful."
#define PORT_FAIL "425 Can't open data connection"
#define FILE_STATUS_OK "150 File status okay; about to open data connection."
#define TRANSFER_COMPLETED "226 Transfer completed."

#define PORT_CMD 0
#define USER_CMD 1
#define PASS_CMD 2
#define STOR_CMD 3
#define RETR_CMD 4
#define LIST_CMD 5
#define CWD_CMD 6
#define PWD_CMD 7
#define C_LIST_CMD 8
#define C_CWD_CMD 9
#define C_PWD_CMD 10
#define QUIT_CMD 11
#define INVALID_CMD -1

#define PORT_REQUEST_FORMAT "PORT %d,%d,%d,%d,%d,%d"
#define RETR_REQUEST_FORMAT "%s %s"
#define IP_ADDR_FORMAT "%d.%d.%d.%d"

#define PACKET_SIZE 2048

#endif