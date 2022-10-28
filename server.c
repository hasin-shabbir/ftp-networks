#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>

#include "common_defs.h"

#include "user_mockDB.c"

int serve_client(int client_fd);
int user_auth(int client_fd);
int request_type(char* request_content);
int handle_user_cmd(char* request_content, int client_fd);
int handle_password_cmd(char* request_content, int client_fd);

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
		if (req_type==1){ //USER
			int result = handle_user_cmd(message, client_fd);
			if (result==1){
				send(client_fd,USER_SUCCESS,strlen(USER_SUCCESS),0);
				printf("user ok sent to client\n"); // TO BE REMOVED
			}
			else if (result==-1){
				send(client_fd,USER_FAILURE,strlen(USER_FAILURE),0);
				printf("user fail sent to client\n"); // TO BE REMOVED
			}
		}
		if (req_type==2){
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
	}

	if(strcmp(message,"bye")==0)
	{
		printf("Client disconnected \n");
		close(client_fd);
		return -1;
	}
	printf("%s \n",message);
	return 0;
}

int request_type(char* request_content){
	char request_buff[MESSAGE_BUFFER_SIZE];
	strcpy(request_buff,request_content);
	char delim[] = " ";
	char* req_type = strtok(request_buff,delim);
	printf("%s",req_type);
	int req_code;

	if (strcmp(req_type, "PORT") == 0){
		req_code = 0;
		return req_code;
	}
	if (strcmp(req_type, "USER") == 0){
		req_code = 1;
		return req_code;
	}
	else if(strcmp(req_type, "PASS") == 0){
		req_code = 2;
		return req_code;
	}
	else if(strcmp(req_type, "STOR") == 0){
		req_code = 3;
		return req_code;
	}
	else if(strcmp(req_type, "RETR") == 0){
		req_code = 4;
		return req_code;
	}
	else if(strcmp(req_type, "LIST") == 0){
		req_code = 5;
		return req_code;
	}
	else if(strcmp(req_type, "CWD") == 0){
		req_code = 6;
		return req_code;
	}
	else if(strcmp(req_type, "PWD") == 0){
		req_code = 7;
		return req_code;
	}
	else if(strcmp(req_type, "!LIST") == 0){
		req_code = 8;
		return req_code;
	}
	else if(strcmp(req_type, "!CWD") == 0){
		req_code = 9;
		return req_code;
	}
	else if(strcmp(req_type, "!PWD") == 0){
		req_code = 10;
		return req_code;
	}
	else if(strcmp(req_type, "QUIT") == 0){
		req_code = 11;
		return req_code;
	}
	else{
		req_code = -1;
		return req_code;
	}
};

int handle_user_cmd(char* request_content, int client_fd){
	char request_buff[MESSAGE_BUFFER_SIZE];
	strcpy(request_buff,request_content);
	char delim[] = " ";
	char* username = strtok(request_buff,delim);
	username = strtok(NULL,delim);
	for (int i=0;i<MAX_USERS;i++){
		if (strcmp(CURRENT_USERS[i].username,username)==0){
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
	printf("pass: %s\n", password);
	for (int i=0;i<MAX_USERS;i++){
		if (CURRENT_USERS[i].id == client_fd && strcmp(CURRENT_USERS[i].password, password)==0){
			return 1;
		}
	}
	return -1;
}