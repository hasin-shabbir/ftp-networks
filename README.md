# Simple FTP client-server application

## Description
This project demonstrates a simplified version of the FTP application protocol, consisting
of two separate programs: an FTP client and an FTP server. The FTP server is responsible for
maintaining FTP sessions and providing file access. The FTP client is split into two components:
an FTP user interface and an FTP client to make requests to the FTP server. The client provides
a simple user interface with a command prompt asking the user to input a command that will be issued to the FTP server.
Note that the FTP server must be started first and supports concurrent connections and data transfers.
## How to

1. Make file by running ```make```
2. The server must be started first before any client so that clients are able to connect.
3. Use the commands listed below

## List of Commands
1. **USER username** This command identifies which user is trying to login to the FTP server. This is
mainly used for authentication, since there might be different access controls specified for each
user. Once the client issues a USER command to the server and the user exists, the server
replies with ”331 Username OK, need password.”, otherwise with "530 Not logged in.".

2. **PASS password** The client needs to authenticate with the user password by issuing a PASS
command followed by the user password. The server replies with either of the following reply
codes: "230 User logged in, proceed." or "530 Not logged in.". The user names and passwords
are saved locally in a file called users.txt and are read by the server program upon
launch.

3. **STOR filename** This command is used to upload a local file named filename from the current client directory to the current server directory.

4. **RETR filename** This command is used to download a file named filename from the current server directory to the current client directory.

5. **LIST** This command is used to list all the files under the current server directory.

6. **!LIST** This command is used to list all the files under the current client directory.

7. **CWD foldername** This command is used to change the current server directory. The server replies with "200 directory changed to pathname/foldername.".

8. **!CWD foldername** This command is used to change the current client directory.

9. **PWD** This command displays the current server directory. The server replies with "257 pathname.".

10. **!PWD** This command displays the current client directory.

11. **QUIT** This command quits the FTP session and closes the control TCP connection. The server replies with "221 Service closing control connection." and the client terminates after receiving the reply. If this command is issued during an ongoing data transfer, the server/client abort the data transfer.


### Note
**PORT h1,h2,h3,h4,p1,p2** This command is used to specify the client IP address and port
number for the data channel and is sent automatically by the client before sending a **RETR**,
**STOR** or **LIST** command. The server replies with "200 PORT command successful." before
starting the data transfer. The argument is the concatenation of a 32-bit internet host address
and a 16-bit TCP port address. This address information is broken into 8-bit fields and the value
of each field is transmitted as a decimal number (in character string representation). The fields
are separated by commas, where h1 is the high order 8 bits of the internet client address and p1
is the high order 8 bits of the client port.
