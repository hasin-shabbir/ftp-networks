all: client server

client: client.c
	gcc  -Wall client.c -o client -pthread

server: server.c 
	gcc -Wall server.c -o server -pthread


clean:
	rm server client
