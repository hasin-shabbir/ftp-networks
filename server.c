#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include <dirent.h>
#include <signal.h>

#include "common_defs.h"
#include "server_state.h"
#include "helper.h"

#include "user_mockDB.c"

int serve_client(int client_fd);
int user_auth(int client_fd);
int request_type(char* request_content);
int handle_user_cmd(char* request_content, int client_fd);
int handle_password_cmd(char* request_content, int client_fd);
int handle_port_cmd(char* request_content, int client_fd);
int handle_retr_cmd(char* request_content, int client_fd);
int handle_stor_cmd(char* request_content, int client_fd);
int set_status(int client_fd);
int handle_list_cmd(char* request_content, int client_fd);
int handle_pwd_cmd(int client_fd);
int handle_cwd_cmd(char* request_content, int client_fd);
int handle_quit_cmd(int client_fd);

int channels = 0; //current data channels (N+i)
int clients = 0; //currently connected clients

int control_sock;

void intHandler(int dummy) {
	for (int i=0;i<MAX_CONNECTIONS;i++){
		SERVER_STATE[i].authenticated = 0;
		SERVER_STATE[i].login_ready = 0;
		SERVER_STATE[i].client_fd = -1;
		strcpy(SERVER_STATE[i].directory,"");
		strcpy(SERVER_STATE[i].username,"");
		SERVER_STATE[i].ftp_port = -1;
		strcpy(SERVER_STATE[i].ip_addr,"");
		//decrease number of connected clients
		clients = 0;

		//close any open data connection for the disconnecting client
		if (SERVER_STATE[i].client_ftp_connection!=-1){
			close(SERVER_STATE[i].client_ftp_connection);
		}
		SERVER_STATE[i].client_ftp_connection = -1;
	}
	close(control_sock);
	
	exit(0);
}

int main()
{
	signal(SIGINT, intHandler);
	//read users db
	if (read_db()==-1){
		perror(DB_READ_FAIL);
		return -1;
	}
	init_users();
	//initialize server state
	init_state();
	
	//1. socket();
	int server_fd = socket(AF_INET,SOCK_STREAM,0);
	control_sock = server_fd;
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
	//2. bind control
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


	//3. listen on control
	if(listen(server_fd,5)<0)
	{
		perror("listen");
		return -1;
	}
	fd_set full_fdset,ready_fdset;
	FD_ZERO(&full_fdset);
	FD_SET(server_fd,&full_fdset);
	int max_fd = server_fd;

	//4. accept on control
	while(1)
	{	
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
					//get a free index in server state
					int free_ind = find_free_ind();
					if (free_ind==-1){
						continue;
					}
					
					int new_fd = accept(server_fd,NULL,NULL);
					printf("client fd = %d \n",new_fd);
					FD_SET(new_fd,&full_fdset);
					//store control port info
					SERVER_STATE[free_ind].client_fd = new_fd;
					//update number of clients
					clients++;

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
	//store message
	char message[MESSAGE_BUFFER_SIZE];	
	bzero(&message,sizeof(message));
	//recv message
	if(recv(client_fd,message,sizeof(message),0)<=0)
	{
		int state_ind = get_state_ind(client_fd);
		SERVER_STATE[state_ind].authenticated = 0;
		SERVER_STATE[state_ind].login_ready = 0;
		SERVER_STATE[state_ind].client_fd = -1;
		strcpy(SERVER_STATE[state_ind].directory,"");
		strcpy(SERVER_STATE[state_ind].username,"");
		SERVER_STATE[state_ind].ftp_port = -1;
		strcpy(SERVER_STATE[state_ind].ip_addr,"");
		//decrease number of connected clients
		clients--;

		//close any open data connection for the disconnecting client
		if (SERVER_STATE[state_ind].client_ftp_connection!=-1){
			close(SERVER_STATE[state_ind].client_ftp_connection);
		}
		SERVER_STATE[state_ind].client_ftp_connection = -1;
		close(client_fd);
		return -1;
	}
	//get REQ type
	int req_type = request_type(message);
	
	if (req_type==QUIT_CMD){ //QUIT
			handle_quit_cmd(client_fd); //call quit handler
			return -1;
	}
	//if server commands needed
	else if (req_type>=0 && req_type<=7){
		if (req_type==USER_CMD){ //USER COMMAND
			//get state index for client
			int state_ind = get_state_ind(client_fd);
			//report if already logged in
			if (SERVER_STATE[state_ind].authenticated==1){
				send(client_fd,ALREADY_LOGGED_IN,strlen(ALREADY_LOGGED_IN),0);
				return 0;
			}
			//call USER handler
			int result = handle_user_cmd(message, client_fd);
			
			//report USER success or failure
			//also update login state (1 means waiting for PASS next)
			if (result==1){
				SERVER_STATE[state_ind].login_ready = 1;
				send(client_fd,USER_SUCCESS,strlen(USER_SUCCESS),0);
			}
			else if (result==-1){
				SERVER_STATE[state_ind].login_ready = 0;
				send(client_fd,USER_FAILURE,strlen(USER_FAILURE),0);
			}
		}
		if (req_type==PASS_CMD){ //PASS COMMAND
			//get state index for client
			int state_ind = get_state_ind(client_fd);
			//report if PASS was not expected
			if (SERVER_STATE[state_ind].login_ready!=1){
				send(client_fd,BAD_SEQUENCE,strlen(BAD_SEQUENCE),0);
				return 0;
			}
			//report if already logged in
			if (SERVER_STATE[state_ind].authenticated==1){
				send(client_fd,ALREADY_LOGGED_IN,strlen(ALREADY_LOGGED_IN),0);
				return 0;
			}
			
			//call password handler
			int result = handle_password_cmd(message, client_fd);
			//report PASS success or failure
			//also update client state or login state
			if (result==1){
				//set status of the client
				set_status(client_fd);
				send(client_fd,PASSWORD_SUCCESS,strlen(PASSWORD_SUCCESS),0);
			}
			else if (result==-1){
				SERVER_STATE[state_ind].login_ready = 0;
				send(client_fd,PASSWORD_FAILURE,strlen(PASSWORD_FAILURE),0);
			}
		}
		if (req_type==PORT_CMD){ //PORT COMMAND
			//get state index for client
			int state_ind = get_state_ind(client_fd);
			//report if unexpected PORT command
			if (SERVER_STATE[state_ind].login_ready==1 && SERVER_STATE[state_ind].authenticated!=1){
				send(client_fd,&(int){-1},sizeof(int),0);
				send(client_fd,BAD_SEQUENCE,strlen(BAD_SEQUENCE),0);
				SERVER_STATE[state_ind].login_ready = 0; //update login state
				return 0;
			}
			//report if unexpected unauthorized attempt
			if (SERVER_STATE[state_ind].authenticated!=1){
				send(client_fd,&(int){-1},sizeof(int),0);
				send(client_fd,NOT_LOGGED_IN,strlen(NOT_LOGGED_IN),0);
				SERVER_STATE[state_ind].login_ready = 0; //update login state
				return 0;
			}

			//call PORT handler
			int result = handle_port_cmd(message, client_fd);
			
			//send PORT success or failure
			if(result==1){
				send(client_fd,PORT_OK,strlen(PORT_OK),0);
			}
			else if(result==-1){
				send(client_fd,PORT_FAIL,strlen(PORT_FAIL),0);
			}
		}

		if (req_type==RETR_CMD){ //RETR COMMAND
			if (fork()==0){//create new process for data exchange
				//call RETR handler
				int result = handle_retr_cmd(message, client_fd);
				//report RETR success
				if (result==1){
					send(client_fd,TRANSFER_COMPLETED,strlen(TRANSFER_COMPLETED),0);
				}
				exit(0); //exit process after data transmission
			}
		}
		if (req_type==STOR_CMD){ //STOR COMMAND
			if (fork()==0){//create new process for data exchange
				//call STOR handler
				int result = handle_stor_cmd(message, client_fd);
				//report STORE success
				if (result==1){
					send(client_fd,TRANSFER_COMPLETED,strlen(TRANSFER_COMPLETED),0);
				}
				exit(0); //exit process after data transmission
			}
		}
		if (req_type==LIST_CMD){ //LIST COMMAND
			if (fork()==0){//create new process for data exchange
				//call LIST handler
				int result = handle_list_cmd(message, client_fd);
				//report LIST success
				if (result==1){
					send(client_fd,TRANSFER_COMPLETED,strlen(TRANSFER_COMPLETED),0);
				}
				exit(0); //exit process after data transmission
			}
		}
		if (req_type==PWD_CMD){ //PWD COMMAND
			//get state index for client
			int state_ind = get_state_ind(client_fd);
			//report if unexpected PWD command
			if (SERVER_STATE[state_ind].login_ready==1 && SERVER_STATE[state_ind].authenticated!=1){				
				send(client_fd,BAD_SEQUENCE,strlen(BAD_SEQUENCE),0);
				SERVER_STATE[state_ind].login_ready = 0; //update login status
				return 0;
			}
			//report if UNAUTHORIZED PWD command
			if (SERVER_STATE[state_ind].authenticated!=1){
				send(client_fd,NOT_LOGGED_IN,strlen(NOT_LOGGED_IN),0);
				SERVER_STATE[state_ind].login_ready = 0; //update login status
				return 0;
			}
			//call PWD handler
			int result = handle_pwd_cmd(client_fd);
			//report PWD failure
			if (result==-1){
				send(client_fd,PWD_FAILURE,strlen(PWD_FAILURE),0);
			}
		}
		if (req_type==CWD_CMD){ //CWD COMMAND
			//get state index for client
			int state_ind = get_state_ind(client_fd);
			//report if unexpected CWD command
			if (SERVER_STATE[state_ind].login_ready==1 && SERVER_STATE[state_ind].authenticated!=1){				
				send(client_fd,BAD_SEQUENCE,strlen(BAD_SEQUENCE),0);
				SERVER_STATE[state_ind].login_ready = 0; //update login status
				return 0;
			}
			//report if UNAUTHORIZED CWD command
			if (SERVER_STATE[state_ind].authenticated!=1){
				send(client_fd,NOT_LOGGED_IN,strlen(NOT_LOGGED_IN),0);
				SERVER_STATE[state_ind].login_ready = 0; //update login status
				return 0;
			}
			//call CWD handler
			int result = handle_cwd_cmd(message, client_fd);
			//report CWD failure
			if (result==-1){
				send(client_fd,	CWD_FAILURE,strlen(CWD_FAILURE),0);
			}
			if (result==-2){
				send(client_fd,	INVALID_DIR,strlen(INVALID_DIR),0);
			}
		}
	}
	else if (req_type==INVALID_CMD){ //INVALID COMMAND
		//get state index for client
		int state_ind = get_state_ind(client_fd);
		//report if unexpected command
		if (SERVER_STATE[state_ind].login_ready==1 && SERVER_STATE[state_ind].authenticated!=1){				
			SERVER_STATE[state_ind].login_ready = 0;
		}
		//report INVALID command
		send(client_fd,INVALID_RES,strlen(INVALID_RES),0);
	}

	return 0;
}

//returns a code for each request type
int request_type(char* request_content){
	//get REQ
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
		printf("inv ret\n");
		return INVALID_CMD;
	}
};

//USER handler
int handle_user_cmd(char* request_content, int client_fd){
	//get username
	char request_buff[MESSAGE_BUFFER_SIZE];
	strcpy(request_buff,request_content);
	char delim[] = " ";
	char* username = strtok(request_buff,delim);
	username = strtok(NULL,delim);
	//report if username missing
	if (username==NULL){
		return -1;
	}
	//update user id with the control socket
	for (int i=0;i<MAX_USERS;i++){
		if (strncmp(CURRENT_USERS[i].username,username,strlen(username)-1)==0){
			int state_ind = get_state_ind(client_fd);
			CURRENT_USERS[i].id = state_ind;
			strcpy(SERVER_STATE[state_ind].username,username);
			return 1;
		}
	}
	return -1;
}

//PASS handler
int handle_password_cmd(char* request_content, int client_fd){
	//get password
	char request_buff[MESSAGE_BUFFER_SIZE];
	strcpy(request_buff,request_content);
	char delim[] = " ";
	char* password = strtok(request_buff,delim);
	password = strtok(NULL,delim);
	//report if password missing
	if (password==NULL){
		return -1;
	}
	int state_ind = get_state_ind(client_fd);
	
	for (int i=0;i<MAX_USERS;i++){
		if (strncmp(CURRENT_USERS[i].username,SERVER_STATE[state_ind].username,strlen(SERVER_STATE[state_ind].username))==0){
				
				if (strncmp(CURRENT_USERS[i].password,password,strlen(password))==0){
					CURRENT_USERS[i].auth = 1;
					return 1;
				}

		}
	}

	return -1;
}

//PORT handler
int handle_port_cmd(char* request_content, int client_fd){
	//increment data channels
	channels++;
	//get state index for the client
	int state_ind = get_state_ind(client_fd);

	//send channel number to client to connect to
	send(client_fd,&channels,sizeof(int),0);

	//create data socket
	int server_data_sock = socket(AF_INET,SOCK_STREAM,0);
	if (server_data_sock<0){
		return -1;
	}

	//store address for data socket
	struct sockaddr_in data_server_addr;	    
	memset(&data_server_addr,0,sizeof(data_server_addr));  
	data_server_addr.sin_family = AF_INET;	            
	data_server_addr.sin_port = htons(DATA_PORT);
	data_server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

	//set socket reusable
	setsockopt(server_data_sock,SOL_SOCKET,SO_REUSEADDR,&(int){1},sizeof(int)); 
	
	//bind
	if(bind(server_data_sock,(struct sockaddr *)&data_server_addr,sizeof(data_server_addr))<0) 
	{
		perror("Bind:");
		return -1;
	}
	
	//receive PORT request
	char PORT_REQ_CONTAINER[MESSAGE_BUFFER_SIZE];
	recv(client_fd,PORT_REQ_CONTAINER,MESSAGE_BUFFER_SIZE,0);

	//parse PORT request
	int h1,h2,h3,h4,p1,p2;
	p2 = -1;
	sscanf(PORT_REQ_CONTAINER,PORT_REQUEST_FORMAT,&h1,&h2,&h3,&h4,&p1,&p2);
	//check if missing request parameters
	if (p2==-1){
		return -1;
	}
	char  client_ip[IP_SIZE];
	sprintf(client_ip, IP_ADDR_FORMAT,h1,h2,h3,h4);
	int client_port = p1*256+p2;
	
	//store client data exchange address
	struct sockaddr_in data_ex_addr; 
	memset(&data_ex_addr, 0, sizeof(data_ex_addr)); 
	data_ex_addr.sin_family = AF_INET;
	data_ex_addr.sin_port = htons(client_port);
	data_ex_addr.sin_addr.s_addr = inet_addr(client_ip);

	//set socket reusable
	// setsockopt(server_data_sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	if (setsockopt(server_data_sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
	{
		perror("setsockopt");
	}

	//connect data exchange socket at the data exchange address
	if (connect(server_data_sock, (struct sockaddr *)&data_ex_addr, sizeof(data_ex_addr)) < 0)
	{
		perror("Connection");
		return -1;
	}

	//store data socket
	SERVER_STATE[state_ind].client_ftp_connection = server_data_sock;

	return 1;
}

//RETR handler
int handle_retr_cmd(char* request_content, int client_fd){
	//get state index for client
	int state_ind = get_state_ind(client_fd);

	//parse RETR req and get filename
	char request_buff[MESSAGE_BUFFER_SIZE];
	strcpy(request_buff,request_content);
	char delim[] = " ";
	char* filename = strtok(request_buff,delim);
	filename = strtok(NULL,delim);

	int connection_at = SERVER_STATE[state_ind].client_ftp_connection;

	if (filename==NULL){
		close(connection_at);
		return -1;
	}

	//send FILE STATUS OK on data socket
	int file_size = get_file_size(filename);
	if (file_size==-1){
		send(connection_at, FILE_STATUS_BAD, strlen(FILE_STATUS_BAD), 0);
		close(connection_at);
		return -1;
	}
	send(connection_at, FILE_STATUS_OK, strlen(FILE_STATUS_OK), 0);


	//get filesize
	if(file_size!=-1){
		
		//open and read file
		FILE* file_to_send = fopen(filename, "r");
		char *file_data = malloc(file_size);
		fread(file_data, 1, file_size, file_to_send); //read the entire file

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
			send(connection_at, file_data + bytes_sent, bites_for_iteration, 0);
			//update sent bytes
			bytes_sent += bites_for_iteration;
		}
		//close data socket after file transmission
		close(connection_at);
		//close file after transmission
		fclose(file_to_send);
	}
	return 1;
}

int handle_stor_cmd(char* request_content, int client_fd){
	//get state index for client
	int state_ind = get_state_ind(client_fd);
	
	//parse STOR req and get filename
	char request_buff[MESSAGE_BUFFER_SIZE];
	strcpy(request_buff,request_content);
	char delim[] = " ";
	char* filename = strtok(request_buff,delim);
	filename = strtok(NULL,delim);


	
	//send FILE STATUS OK on data socket
	int connection_at = SERVER_STATE[state_ind].client_ftp_connection;

	//report if missing filename
	if (filename==NULL){
		send(connection_at, FILE_STATUS_BAD, strlen(FILE_STATUS_BAD), 0);
		close(connection_at);
		return -1;
	}
	send(connection_at, FILE_STATUS_OK, strlen(FILE_STATUS_OK), 0);


	//buffer to receive file
	char buffer[PACKET_SIZE];
	//temporary filename
	char temp_fname[FILE_NAME_MAX];
	sprintf(temp_fname,"temp%d.dat",channels);

	//check if file writable and create new file
    FILE* file_to_recv = fopen(temp_fname, "w");
    if (file_to_recv == NULL) {
        return -1;
    }
    fclose(file_to_recv);
	
	//file pointer to use for writing
    file_to_recv = fopen(temp_fname, "a+");
	
	//keep track of received bytes
    int bytes_received = recv(connection_at, buffer, sizeof(buffer), 0);
	
	// if nothing received, terminate action
    if (bytes_received == 0){
		return -1;
	}
	
	//while something is received
    while (bytes_received > 0) {
		//write to file
        fwrite(buffer, bytes_received, 1, file_to_recv);
        bytes_received = recv(connection_at, buffer, sizeof(buffer), 0);
    }
	
	//close data socket upon file transmission
	close(connection_at);
	//close file upon file transmission
	fclose(file_to_recv);
	//rename file to specified name
	rename(temp_fname,filename);
    
	return 1;
}

//set status for client on login
int set_status(int client_fd){
	//get state index for client
	int state_ind = get_state_ind(client_fd);
	//set current directory
	char client_directory[DIRECTORY_NAME_MAX];
	getcwd(client_directory, sizeof(client_directory));
	strcpy(SERVER_STATE[state_ind].directory, client_directory);
	//set auth status to 1
	SERVER_STATE[state_ind].authenticated = 1;
	//set data sock id to -1
	SERVER_STATE[state_ind].client_ftp_connection = -1;	
	return 1;
}

//LIST handler
int handle_list_cmd(char* request_content, int client_fd){
	//get state index for client
	int state_ind = get_state_ind(client_fd);

	//open current directory
	DIR* directory;
	struct dirent *dir;
	directory = opendir(SERVER_STATE[state_ind].directory);

	//send FILE STATUS OK over data socket
	int connection_at = SERVER_STATE[state_ind].client_ftp_connection;
	send(connection_at, FILE_STATUS_OK, strlen(FILE_STATUS_OK), 0);

	//store list of files
	char listBuff[DIRECTORY_LIST_MAX];

	if (directory){
		while((dir=readdir(directory))!=NULL){
			strcat(listBuff,dir->d_name);
			strcat(listBuff,"\n");
		}
		//send directory file info to client
		int size_list = sizeof(listBuff);
		send(connection_at,&size_list,sizeof(int),0);
		send(connection_at,listBuff,size_list,0);
		//close directory
		closedir(directory);
	}
	else{
		//report if directory doesn't exist
		return -1;
	}
	return 1;
}

//PWD handler
int handle_pwd_cmd(int client_fd){
	//get state index for client
	int state_ind = get_state_ind(client_fd);
	//get current directory for client
	char directory_name[DIRECTORY_NAME_MAX];
	strcpy(directory_name,SERVER_STATE[state_ind].directory);
	//report if error
	if (directory_name==NULL){
		return -1;
	}
	//send PWD OK response
	char RESPONSE_CONTENT[MESSAGE_BUFFER_SIZE];
	snprintf(RESPONSE_CONTENT,MESSAGE_BUFFER_SIZE,PWD_SUCCESS_FORMAT, directory_name);
	send(client_fd,RESPONSE_CONTENT,strlen(RESPONSE_CONTENT),0);	
	return 1;
}

int handle_cwd_cmd(char* request_content, int client_fd){
	//get state index for client
	int state_ind = get_state_ind(client_fd);

	//get directory name from request
	char request_buff[MESSAGE_BUFFER_SIZE];
	strcpy(request_buff,request_content);
	char delim[] = " ";
	char* dirname = strtok(request_buff,delim);
	dirname = strtok(NULL,delim);

	//report if missing directory name
	if (dirname==NULL){
		return -1;
	}
	
	//change directory and report if any error
	if(chdir(dirname)==-1){
		return -2;
	}

	//get current directory
	getcwd(SERVER_STATE[state_ind].directory,sizeof(SERVER_STATE[state_ind].directory));
	
	//respond with current directory
	char RESPONSE_CONTENT[MESSAGE_BUFFER_SIZE];
	snprintf(RESPONSE_CONTENT,MESSAGE_BUFFER_SIZE,CWD_SUCCESS_FORMAT, SERVER_STATE[state_ind].directory);
	send(client_fd,RESPONSE_CONTENT,strlen(RESPONSE_CONTENT),0);
	return 1;
}

int handle_quit_cmd(int client_fd){
	//get status index for client
	int state_ind = get_state_ind(client_fd);
	//reset server state for that index
	SERVER_STATE[state_ind].authenticated = 0;
	SERVER_STATE[state_ind].login_ready = 0;
	SERVER_STATE[state_ind].client_fd = -1;
	strcpy(SERVER_STATE[state_ind].directory,"");
	strcpy(SERVER_STATE[state_ind].username,"");
	SERVER_STATE[state_ind].ftp_port = -1;
	strcpy(SERVER_STATE[state_ind].ip_addr,"");
	//decrease number of connected clients
	clients--;

	//close any open data connection for the disconnecting client
	if (SERVER_STATE[state_ind].client_ftp_connection!=-1){
		close(SERVER_STATE[state_ind].client_ftp_connection);
	}
	SERVER_STATE[state_ind].client_ftp_connection = -1;
	//respond with QUIT OK
	send(client_fd,QUIT_MESSAGE,strlen(QUIT_MESSAGE),0);
	//close control socket
	close(client_fd);
	return 1;
}