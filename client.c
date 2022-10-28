#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <netinet/in.h>

#include<unistd.h>
#include<stdlib.h>

#include "common_defs.h"


void instructions();
void user_login(int server_sd);

int main()
{
	//socket
	int server_sd = socket(AF_INET,SOCK_STREAM,0);
	if(server_sd<0)
	{
		perror("socket:");
		exit(-1);
	}
	//setsock
	int value  = 1;
	setsockopt(server_sd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value)); //&(int){1},sizeof(int)
	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(CONTROL_PORT);
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP); //INADDR_ANY, INADDR_LOOP

	//connect
    if(connect(server_sd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        perror("connect");
        exit(-1);
    }
    printf("Connected to server\n");
	// printf("ftp> ");
	
	//accept
	char buffer[MESSAGE_BUFFER_SIZE];

	instructions();
	user_login(server_sd);

	while(1)
	{
       fgets(buffer,sizeof(buffer),stdin);
       printf("ftp> ");
       buffer[strcspn(buffer, "\n")] = 0;  //remove trailing newline char from buffer, fgets does not remove it
       if(strncmp(buffer,"QUIT", 4)==0)
        {
			send(server_sd,buffer,strlen(buffer),0);
        	rcvd(server_sd, buffer, sizeof(buffer), 0);
			printf("%s\n", buffer);
			bzero(buffer,sizeof(buffer));
        	close(server_sd);
            break;
        }
        if(send(server_sd,buffer,strlen(buffer),0)<0)
        {
            perror("send");
            exit(-1);
        }
        
		else if(strncmp(buffer, "STOR", 4)==0 || strncmp(buffer, "RETR", 4)==0 || strncmp(buffer, "LIST", 4)==0)
		{
			int channel = 1;
			
		}			
	}

	return 0;
}

void instructions()
{
	printf("Welcome! Please authenticate user to be able to run server commands.\n");
	printf("List of commands:\n");
	printf("USER -> enter username\n");
	printf("PASS -> enter password\n");
	printf("STOR -> upload a file from local directory to curren server directory\n");
	printf("RETR -> download file from current server directory to current local directory\n");
	printf("LIST -> list all files in current server directory\n");
	printf("!LIST -> list all files in current local directory\n");
	printf("CWD -> change current server directory\n");
	printf("!CWD -> change current local directory\n");
	printf("PWD -> display current server directory\n");
	printf("!PWD -> display current local directory\n");
	printf("QUIT -> close ftp session\n");
}

void user_login(int server_sd)
{
	char buffer[MESSAGE_BUFFER_SIZE];
	// char* command1;
	// char* command2;
	char* error = "Login failed. Please try again.";

	while (1)
	{
		bzero(buffer, sizeof(buffer));
		printf("ftp> ");
		fgets(buffer, MESSAGE_BUFFER_SIZE, stdin);
		// command1 = strtok(buffer, " ");
		// command2 = strtok(NULL, "\n");

		if(strncmp(buffer, "USER", 4)==0)
		{
			int sent = send(server_sd,buffer,sizeof(buffer),0);
			bzero(buffer, sizeof(buffer));
			int rcvd = recv(server_sd,buffer,sizeof(buffer),0);
			printf("%s\n", buffer);
		}
		else
		{
			printf("%s\n", error);
			continue;
		}

		if(strncmp(buffer, "331", 3)==0)
		{
			bzero(buffer, sizeof(buffer));
			printf("ftp> ");
			fgets(buffer, MESSAGE_BUFFER_SIZE, stdin);
			// command1 = strtok(buffer, " ");
			// command2 = strtok(NULL, "\n");

			if(strncmp(buffer, "PASS", 4)==0)
			{
				int sent = send(server_sd, buffer, sizeof(buffer), 0);
				bzero(buffer, sizeof(buffer));
				int rcvd = recv(server_sd, buffer, sizeof(buffer), 0);
				printf("%s\n", buffer);

				if(strncmp(buffer, "230", 3)==0)
				{
					return;
				}
			}
			else
			{
				printf("%s\n", error);
			}
		}
	}
}