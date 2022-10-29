#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>

#include "common_defs.h"
#include "server_state.h"

#include "user_mockDB.c"
#include "helper.c"

int serve_client(int client_fd);
int user_auth(int client_fd);
int request_type(char* request_content);
int handle_user_cmd(char* request_content, int client_fd);
int handle_password_cmd(char* request_content, int client_fd);
int handle_port_cmd(char* request_content, int client_fd);
int handle_retr_cmd(char* request_content, int client_fd);

int main()
{
	//read users db
	if (read_db()==-1){
		perror(DB_READ_FAIL);
		return -1;
	}
	//1. socket();
	int server_fd = socket(AF_INET,SOCK_STREAM,0);
	printf("server_fd = %d \n",server_fd);
	if(server_fd<0)
	{
		perror("socket");
		return -1;
	}
	if(setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&(int){1},sizeof(int))<0)
	{
		perror("setsock");
		return -1;
	}
	//2. bind ();
	struct sockaddr_in server_address;
	bzero(&server_address,sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(CONTROL_PORT);
	server_address.sin_addr.s_addr = inet_addr(SERVER_IP);
	if(bind(server_fd,(struct sockaddr*)&server_address,sizeof(server_address))<0)
	{
		perror("bind");
		return -1;
	}


	//3. listen()
	if(listen(server_fd,5)<0)
	{
		perror("listen");
		return -1;
	}
	fd_set full_fdset,ready_fdset;
	FD_ZERO(&full_fdset);
	FD_SET(server_fd,&full_fdset);
	int max_fd = server_fd;

	//4. accept()
	while(1)
	{	
		// printf("max_fd=%d\n",max_fd);
		ready_fdset = full_fdset;
		if(select(max_fd+1,&ready_fdset,NULL,NULL,NULL)<0)
		{
			perror("select");
			return -1;
		}

		for(int fd = 0; fd<=max_fd; fd++)
		{
			if(FD_ISSET(fd,&ready_fdset))
			{
				if(fd==server_fd) 
				{
					int new_fd = accept(server_fd,NULL,NULL);
					printf("client fd = %d \n",new_fd);
					FD_SET(new_fd,&full_fdset);
					if(new_fd>max_fd) max_fd=new_fd;    //Update the max_fd if new socket has higher FD
				}
				else if(serve_client(fd)==-1)
				{
					FD_CLR(fd,&full_fdset);
				}
			}
		}
	}

	//6. close());
	close(server_fd);
}
//=================================
int serve_client(int client_fd){
	char message[MESSAGE_BUFFER_SIZE];	
	bzero(&message,sizeof(message));
	if(recv(client_fd,message,sizeof(message),0)<0)
	{
		perror("recv");
		return 0;
	}
	int req_type = request_type(message);
	printf("%d\n",req_type);
	
	int is_auth = 0;
	if (req_type>=1 && req_type<=7){
		if (req_type==USER_CMD){ //USER
			int result = handle_user_cmd(message, client_fd);
			if (result==USER_CMD){
				send(client_fd,USER_SUCCESS,strlen(USER_SUCCESS),0);
				printf("user ok sent to client\n"); // TO BE REMOVED
			}
			else if (result==-1){
				send(client_fd,USER_FAILURE,strlen(USER_FAILURE),0);
				printf("user fail sent to client\n"); // TO BE REMOVED
			}
		}
		if (req_type==PASS_CMD){ //PASS
			int result = handle_password_cmd(message, client_fd);
			if (result==1){
				send(client_fd,PASSWORD_SUCCESS,strlen(PASSWORD_SUCCESS),0);
				printf("password ok sent to client\n"); // TO BE REMOVED
			}
			else if (result==-1){
				send(client_fd,PASSWORD_FAILURE,strlen(PASSWORD_FAILURE),0);
				printf("password fail sent to client\n"); // TO BE REMOVED
			}
		}
		if (req_type==QUIT_CMD){ //QUIT
			printf("Client disconnected \n");
			send(client_fd,QUIT_MESSAGE,strlen(QUIT_MESSAGE),0);
			close(client_fd);
			return -1;
		}
		if (req_type==PORT_CMD){
			int result = handle_port_cmd(message, client_fd);
			if(result==1){
				send(client_fd,PORT_OK,strlen(PORT_OK),0);
				printf("port OK sent to client\n");
			}
			else if(result==-1){
				send(client_fd,PORT_FAIL,strlen(PORT_FAIL),0);
				printf("port FAIL sent to client\n");
			}
		}
		if (req_type==RETR_CMD){
			int result = handle_retr_cmd(message, client_fd);
			if (result==1){
				send(client_fd,TRANSFER_COMPLETED,strlen(TRANSFER_COMPLETED),0);
			}
		}
	}

	printf("%s \n",message);
	return 0;
}

int request_type(char* request_content){
	char request_buff[MESSAGE_BUFFER_SIZE];
	strcpy(request_buff,request_content);
	char delim[] = " ";
	char* req_type = strtok(request_buff,delim);

	if (strcmp(req_type, "PORT") == 0){
		return PORT_CMD;
	}
	if (strcmp(req_type, "USER") == 0){
		return USER_CMD;
	}
	else if(strcmp(req_type, "PASS") == 0){
		return PASS_CMD;
	}
	else if(strcmp(req_type, "STOR") == 0){
		return STOR_CMD;
	}
	else if(strcmp(req_type, "RETR") == 0){
		return RETR_CMD;
	}
	else if(strcmp(req_type, "LIST") == 0){
		return LIST_CMD;
	}
	else if(strcmp(req_type, "CWD") == 0){
		return CWD_CMD;
	}
	else if(strcmp(req_type, "PWD") == 0){
		return PWD_CMD;
	}
	else if(strcmp(req_type, "!LIST") == 0){
		return C_LIST_CMD;
	}
	else if(strcmp(req_type, "!CWD") == 0){
		return C_CWD_CMD;
	}
	else if(strcmp(req_type, "!PWD") == 0){
		return C_PWD_CMD;
	}
	else if(strcmp(req_type, "QUIT") == 0){
		return QUIT_CMD;
	}
	else{
		return INVALID_CMD;
	}
};

int handle_user_cmd(char* request_content, int client_fd){
	char request_buff[MESSAGE_BUFFER_SIZE];
	strcpy(request_buff,request_content);
	char delim[] = " ";
	char* username = strtok(request_buff,delim);
	username = strtok(NULL,delim);
	for (int i=0;i<MAX_USERS;i++){
		if (strncmp(CURRENT_USERS[i].username,username,strlen(username)-1)==0){
			CURRENT_USERS[i].id = client_fd;
			return 1;
		}
	}
	return -1;
}

int handle_password_cmd(char* request_content, int client_fd){
	char request_buff[MESSAGE_BUFFER_SIZE];
	strcpy(request_buff,request_content);
	char delim[] = " ";
	char* password = strtok(request_buff,delim);
	password = strtok(NULL,delim);
	for (int i=0;i<MAX_USERS;i++){
		if (CURRENT_USERS[i].id == client_fd && strncmp(CURRENT_USERS[i].password, password, strlen(password)-1)==0){
			return 1;
		}
	}
	return -1;
}

int handle_port_cmd(char* request_content, int client_fd){
	//multiple connections, same client

	int client_ftp_connection = socket(AF_INET, SOCK_STREAM, 0);
	if (setsockopt(client_ftp_connection, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0){
        perror("setsockopt"); 
        return -1;
    }
	if (client_ftp_connection < 0){
        perror("socket");
        return -1;
    }
	
	struct sockaddr_in client_ftp_connection_addr;
	bzero(&client_ftp_connection_addr, sizeof(client_ftp_connection_addr));
	client_ftp_connection_addr.sin_family = AF_INET;
    client_ftp_connection_addr.sin_port = htons(DATA_PORT); //increment port num on connections
    client_ftp_connection_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
 	
	if (bind(client_ftp_connection, (struct sockaddr *)&client_ftp_connection_addr, sizeof(client_ftp_connection_addr)) < 0){
        perror("bind");
        return -1;
    }

	int h1, h2, h3, h4, p1, p2;

	int state_ind = curr_states;
	if (state_ind==MAX_CONNECTIONS){
		return -1;
	}
	curr_states++;
	SERVER_STATE[state_ind].client_fd = client_fd;
	
	sscanf(request_content, PORT_REQUEST_FORMAT, &h1, &h2, &h3, &h4, &p1, &p2);
	SERVER_STATE[state_ind].ftp_port = p1 * 256 + p2;

    snprintf(SERVER_STATE[state_ind].ip_addr, IP_SIZE, IP_ADDR_FORMAT, h1, h2, h3, h4);

    SERVER_STATE[state_ind].client_ftp_connection = client_ftp_connection;

	return 1;
}

int handle_retr_cmd(char* request_content, int client_fd){
	int state_ind = get_state_ind(client_fd);

	char request_buff[MESSAGE_BUFFER_SIZE];
	strcpy(request_buff,request_content);
	char delim[] = " ";
	char* filename = strtok(request_buff,delim);
	filename = strtok(NULL,delim);

	if (filename==NULL){
		return -1;
	}

	struct sockaddr_in client_connection;

	bzero(&client_connection, sizeof(client_connection));
    client_connection.sin_family = AF_INET;
    client_connection.sin_port = htons(SERVER_STATE[state_ind].ftp_port);
    client_connection.sin_addr.s_addr = inet_addr(SERVER_STATE[state_ind].ip_addr);

	send(client_fd, FILE_STATUS_OK, strlen(FILE_STATUS_OK), 0);

	if (connect(SERVER_STATE[state_ind].client_ftp_connection, (struct sockaddr *)&client_connection, sizeof(client_connection)) < 0)
    {
        perror("connect");

        return -1;
    }

	int file_size = get_file_size(filename);
	if(file_size!=-1){
		//inform about the length of file to be received
		send(SERVER_STATE[state_ind].client_ftp_connection, &file_size, sizeof(int), 0);

		FILE* file_to_send = fopen(filename, "r");
        char *file_data = malloc(file_size);
        fread(file_data, 1, file_size, file_to_send); //read the entire file

        int bytes_sent = 0;
        int bites_for_iteration = 0;
        while(bytes_sent < file_size)
        {
            if(file_size - bytes_sent >= PACKET_SIZE){
                bites_for_iteration = PACKET_SIZE;
			}
            else{
                bites_for_iteration = file_size - bytes_sent;
			}
            send(SERVER_STATE[state_ind].client_ftp_connection, file_data + bytes_sent, bites_for_iteration, 0);
            bytes_sent += bites_for_iteration;
        }

		close(SERVER_STATE[state_ind].client_ftp_connection);
		fclose(file_to_send);
		return 0;
	}
	else{ //file doesn't exist
		return -1;
	}

}
