#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <netinet/in.h>
#include<unistd.h>
#include<stdlib.h>
#include<dirent.h>

#include "common_defs.h"
#include "helper.h"


void instructions();

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
	setsockopt(server_sd,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value));
	// store address and port
	struct sockaddr_in server_addr;
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(CONTROL_PORT);
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

	//connect
    if(connect(server_sd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        perror("connect");
        exit(-1);
    }
    printf("Connected to server\n");
	
	//accept
	char buffer[MESSAGE_BUFFER_SIZE];

	//print instructions for client
	instructions();

	while(1)
	{
		printf("ftp> ");
		//read input command
		fgets(buffer,sizeof(buffer),stdin);
		buffer[strcspn(buffer, "\n")] = 0;  //remove trailing newline char from buffer, fgets does not remove it
		
		// if QUIT, send and recv response from server and close socket
		if(strncmp(buffer,"QUIT", 4)==0)
        {
			send(server_sd,buffer,strlen(buffer),0);
			bzero(buffer,sizeof(buffer));
        	int rec_bytes = recv(server_sd, buffer, sizeof(buffer), 0);
			//check if server shutdown
			if (rec_bytes<=0){
				printf("Server has shutdown\n");
				return 0;
			}
			printf("%s\n", buffer);
        	close(server_sd);
            break;
        }
        
		//if PORT required
		else if(strncmp(buffer, "STOR", 4)==0 || strncmp(buffer, "RETR", 4)==0 || strncmp(buffer, "LIST", 4)==0)
		{
			//check missing filename argument and report
			if (strncmp(buffer, "STOR", 4)==0 || strncmp(buffer, "RETR", 4)==0){
				char reject_buff[MESSAGE_BUFFER_SIZE];
				strcpy(reject_buff,buffer);
				char delim[] = " ";
				char* reject_filename = strtok(reject_buff,delim);
				reject_filename = strtok(NULL,delim);

				if (reject_filename==NULL){
					printf("PROVIDE FILENAME\n");
					continue;
				}
			}

			//inform server about incoming port command
			send(server_sd,"PORT",4,0);
			
			//channel on data port to connect to (N+i)
			int channel;
			int rec_bytes = recv(server_sd,&channel,sizeof(channel),0);
			//check if server shutdown
			if (rec_bytes<=0){
				printf("Server has shutdown\n");
				return 0;
			}
			//stop unauthorized access on commands using PORT
			if (channel==-1){
				char PORT_REJECT_CONTAINER[MESSAGE_BUFFER_SIZE];
				recv(server_sd,PORT_REJECT_CONTAINER,MESSAGE_BUFFER_SIZE,0);
				printf("%s\n",PORT_REJECT_CONTAINER);
				continue;
			}

			//address of where to make data connection
			struct sockaddr_in curr_addr;
			socklen_t curr_addr_size = sizeof(curr_addr);

			if (getsockname(server_sd, (struct sockaddr*) &curr_addr, &curr_addr_size) != 0) {
				perror("Socket: ");
				continue;
			}

			//set port and ip for connection
			char PORT_REQ_CONTAINER[MESSAGE_BUFFER_SIZE];
			int h1,h2,h3,h4,p1,p2;
			int client_port = ntohs(curr_addr.sin_port)+channel;
			char* client_ip = inet_ntoa(curr_addr.sin_addr);
			p1 = client_port/256;
			p2 = client_port%256;
			sscanf(client_ip,IP_ADDR_FORMAT,&h1,&h2,&h3,&h4);
			snprintf(PORT_REQ_CONTAINER,MESSAGE_BUFFER_SIZE,PORT_REQUEST_FORMAT,h1,h2,h3,h4,p1,p2);

			//send PORT request with relevant params
			send(server_sd,PORT_REQ_CONTAINER,MESSAGE_BUFFER_SIZE,0);

			//create socket for data exchange
			int client_data_sock = socket(AF_INET,SOCK_STREAM,0);

			if (client_data_sock<0){
				perror("data sock: ");
				continue;
			}
			
			//set address for clients data exchange socket
			struct sockaddr_in data_ex_addr;
            memset(&data_ex_addr, 0, sizeof(data_ex_addr));
            data_ex_addr.sin_family = AF_INET;
            data_ex_addr.sin_port = htons(client_port);
            data_ex_addr.sin_addr.s_addr = inet_addr(client_ip);
            // socklen_t data_ex_addr_size = sizeof(data_ex_addr);
            if (setsockopt(client_data_sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
            {
                perror("setsockopt");
            }
			
			//bind socket to the data exchange address
            if (bind(client_data_sock, (struct sockaddr *)&data_ex_addr, sizeof(data_ex_addr)) < 0)
            {
                perror("bind");
                continue;
            }
			//listen on the data exchange socket
            if (listen(client_data_sock, 5) < 0)
            {
                perror("listen");
                close(client_data_sock);
				continue;
            }
			
			//recv acknowledgement of PORT request
			char PORT_RES_CONTAINER[MESSAGE_BUFFER_SIZE];
			recv(server_sd,PORT_RES_CONTAINER,MESSAGE_BUFFER_SIZE,0);
			printf("%s\n",PORT_RES_CONTAINER);
			//terminate action if PORT failed
			if (strncmp(PORT_RES_CONTAINER,"200",3)!=0){
				continue;
			}

			//socket to connect with server
			int server_data_sock = accept(client_data_sock, 0, 0);

			if (server_data_sock < 1)
            {
                perror("accept");
                continue;
            }

			////////////////////////////////////////////////
			///////////////////PORT END////////////////////
			//////////////////////////////////////////////
			
			//deal with STOR req
			if(strncmp(buffer, "STOR", 4)==0){
				//get filename
				char request_buff[MESSAGE_BUFFER_SIZE];
				strcpy(request_buff,buffer);
				char delim[] = " ";
				char* filename = strtok(request_buff,delim);
				filename = strtok(NULL,delim);

				//send STOR req to server on control socket
				int file_size = get_file_size(filename);
				if (file_size==-1){
					send(server_sd,"STOR",4,0);
				}
				else{
					send(server_sd,buffer,sizeof(buffer),0);
				}
				
				//recv DATA OK response
				char data_ok_buff[MESSAGE_BUFFER_SIZE];
				recv(server_data_sock,data_ok_buff,sizeof(data_ok_buff),0);
				printf("%s\n",data_ok_buff);
				if (strncmp(data_ok_buff,"550",3)==0){
					close(server_data_sock);
					continue;
				}
				
				


				if(file_size!=-1){
					
					//open file for read
					FILE* file_to_send = fopen(filename, "r");
					char *file_data = malloc(file_size);
					//read the entire file
					fread(file_data, 1, file_size, file_to_send);


					//keep track of bytes sent and bytes for the iteration
					int bytes_sent = 0;
					int bites_for_iteration = 0;
					while(bytes_sent < file_size)
					{
						//if remaining file size is greater than packet size, then send packet sized data
						if(file_size - bytes_sent >= PACKET_SIZE){
							bites_for_iteration = PACKET_SIZE;
						}
						//else send remaining data
						else{
							bites_for_iteration = file_size - bytes_sent;
						}
						//send data on the data socket
						send(server_data_sock, file_data + bytes_sent, bites_for_iteration, 0);
						//update sent bytes
						bytes_sent += bites_for_iteration;

					}
					//close data socket after file transmission
					close(server_data_sock);
					//close file after file transmission
					fclose(file_to_send);
				}
				//recv STOR response
				bzero(request_buff,sizeof(request_buff));
				recv(server_sd,request_buff,sizeof(request_buff),0);
				printf("%s\n",request_buff);
			}

			if(strncmp(buffer, "RETR", 4)==0){

				//get filename from request
				char request_buff[MESSAGE_BUFFER_SIZE];
				strcpy(request_buff,buffer);
				char delim[] = " ";
				char* filename = strtok(request_buff,delim);
				filename = strtok(NULL,delim);
				
				//send RETR request to server on control socket
				send(server_sd,buffer,sizeof(buffer),0);
				
				//receive DATA OK response
				char data_ok_buff[MESSAGE_BUFFER_SIZE];
				recv(server_data_sock,data_ok_buff,sizeof(data_ok_buff),0);
				printf("%s\n",data_ok_buff);
				if (strncmp(data_ok_buff,"550",3)==0){
					close(server_data_sock);
					continue;
				}

				//buffer to receive file
				char f_recv_buff[PACKET_SIZE];
				//temporary filename
				char temp_fname[FILE_NAME_MAX];
				sprintf(temp_fname,"temp%d.dat",channel);

				//check if file writable and create new file
				FILE* file_to_recv = fopen(temp_fname, "w");
				if (file_to_recv == NULL) {
					continue;
				}
				fclose(file_to_recv);

				//file pointer to use for writing
				file_to_recv = fopen(temp_fname, "a+");

				//keep track of received bytes
				int bytes_received = recv(server_data_sock, f_recv_buff, sizeof(f_recv_buff), 0);
				
				// if nothing received, terminate action
				if (bytes_received <= 0){
					continue;
				}
				//while something is received
				while (bytes_received > 0) {
					//write to file
					fwrite(f_recv_buff, bytes_received, 1, file_to_recv);
					//stop if last transmission
					if (bytes_received<PACKET_SIZE){
						break;
					}
					bytes_received = recv(server_data_sock, f_recv_buff, sizeof(f_recv_buff), 0);
				}
				//close data socket upon file transmission
				close(server_data_sock);
				//close file upon file transmission
				fclose(file_to_recv);
				//rename file to specified name
				rename(temp_fname,filename);

				//receive retrieve OK
				bzero(request_buff,sizeof(request_buff));
				recv(server_sd,request_buff,sizeof(request_buff),0);
				printf("%s\n",request_buff);
			}
			if (strncmp(buffer,"LIST",4)==0){
				//send LIST on control socket
				send(server_sd,buffer,sizeof(buffer),0);

				//receive DATA OK
				char data_ok_buff[MESSAGE_BUFFER_SIZE];
				recv(server_data_sock,data_ok_buff,sizeof(data_ok_buff),0);
				printf("%s\n",data_ok_buff);

				//receive and report the directory file info
				int size_incoming;
				char listBuff[DIRECTORY_LIST_MAX];
				recv(server_data_sock,&size_incoming,sizeof(int),0);
				recv(server_data_sock,listBuff,size_incoming,0);
				printf("%s\n",listBuff);
				//close data socket upon file transmission
				close(server_data_sock);

				//receive LIST OK
				char request_buff[MESSAGE_BUFFER_SIZE];
				bzero(request_buff,sizeof(request_buff));
				recv(server_sd,request_buff,sizeof(request_buff),0);
				printf("%s\n",request_buff);
			}
			
		}			
		else if (strncmp(buffer,"PWD",3)==0){
			//send PWD on control socket
			send(server_sd,buffer,sizeof(buffer),0);

			//receive and report response
			char request_buff[MESSAGE_BUFFER_SIZE];
			bzero(request_buff,sizeof(request_buff));
			int rec_bytes = recv(server_sd,request_buff,sizeof(request_buff),0);
			if (rec_bytes<=0){
				printf("Server has shutdown\n");
				return 0;
			}
			printf("%s\n",request_buff);
		}
		else if (strncmp(buffer,"CWD",3)==0){
			//send CWD on control socket
			send(server_sd,buffer,sizeof(buffer),0);
			
			//receive and report response
			char request_buff[MESSAGE_BUFFER_SIZE];
			bzero(request_buff,sizeof(request_buff));
			int rec_bytes = recv(server_sd,request_buff,sizeof(request_buff),0);
			if (rec_bytes<=0){
				printf("Server has shutdown\n");
				return 0;
			}
			printf("%s\n",request_buff);
		}
		else if(strncmp(buffer,"!CWD",4)==0){
			//get name of directory
			char temp_buff[MESSAGE_BUFFER_SIZE];
			strcpy(temp_buff,buffer);
			char delim[] = " ";
			char* dirname = strtok(temp_buff,delim);
			dirname = strtok(NULL,delim);

			//report if dir name not provided
			if (dirname==NULL){
				printf("DIRECTORY NAME NOT PROVIDED\n");
				continue;
			}
			//change directory and report if any error
			if(chdir(dirname)==-1){
				printf("!CWD FAILED\n");
				continue;
			}
			//get and print current directory
			bzero(temp_buff,sizeof(temp_buff));
			getcwd(temp_buff,sizeof(temp_buff));
			printf("client directory changed to %s.\n",temp_buff);
		}
		else if(strncmp(buffer,"!PWD",4)==0){
			//get directory name
			char temp_buff[DIRECTORY_NAME_MAX];
			getcwd(temp_buff,sizeof(temp_buff));
			//report error
			if (temp_buff==NULL){
				printf("Error in !PWD\n");
				continue;
			}
			//report directory name
			printf("client directory: %s.\n",temp_buff);
		}
		else if(strncmp(buffer,"!LIST",4)==0){
			//get directory name
			char temp_buff[DIRECTORY_NAME_MAX];
			getcwd(temp_buff,sizeof(temp_buff));
			//report error
			if (temp_buff==NULL){
				printf("Error in !LIST\n");
				continue;
			}

			//open directory
			DIR* directory;
			struct dirent *dir;
			directory = opendir(temp_buff);
			//report directory files
			if (directory){
				while((dir=readdir(directory))!=NULL){
					printf("%s\n",dir->d_name);
				}
				//close directory
				closedir(directory);
			}
			//report error
			else{
				printf("!LIST failed");
				continue;
			}
		}
		else if (strncmp(buffer,"USER",4)==0){
			//get username
			
			
			//send USER req on control socket
			send(server_sd,buffer,sizeof(buffer),0);
			bzero(buffer, sizeof(buffer));
			//recv USER resp through control socket
			int rec_bytes = recv(server_sd,buffer,sizeof(buffer),0);
			if (rec_bytes<=0){
				printf("Server has shutdown\n");
				return 0;
			}
			printf("%s\n", buffer);
		}
		else if (strncmp(buffer,"PASS",4)==0){

			//send PASS req on control socket
			send(server_sd,buffer,sizeof(buffer),0);
			bzero(buffer, sizeof(buffer));
			//recv PASS resp through control socket
			int rec_bytes = recv(server_sd,buffer,sizeof(buffer),0);
			if (rec_bytes<=0){
				printf("Server has shutdown\n");
				return 0;
			}
			printf("%s\n", buffer);
		}
		else{
			//send INVALID request info
			send(server_sd,INVALID_REQ,strlen(INVALID_REQ),0);
			//recv and report INVALID resp
			char request_buff[MESSAGE_BUFFER_SIZE];
			bzero(request_buff,sizeof(request_buff));
			int rec_bytes = recv(server_sd,request_buff,sizeof(request_buff),0);
			if (rec_bytes<=0){
				printf("Server has shutdown\n");
				return 0;
			}
			printf("%s\n",request_buff);
		}
	}

	return 0;
}


//instructions for client
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