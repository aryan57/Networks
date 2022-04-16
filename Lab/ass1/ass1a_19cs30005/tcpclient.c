/*
	Aryan Agarwal, 19CS30005
	Networks Lab Assgn1
	tcpclient.c
*/
/*    THE CLIENT PROCESS */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFF_SIZE 100
#define CHUNK_SIZE 10
#define PORT 7000

int main(int argc, char *argv[])
{
	/*
		since the file name has to be given from command line,
		there must be atleast 2 arguments (everything after 2nd argument will be ignored).
		Example: ./tcpclient foo.txt
	*/
	if (argc < 2)
	{
		perror("Filename not in command.\n");
		exit(0);
	}

	/*
		open the file as read only,
		using the open() system call
	*/
	int fd = open(argv[1], O_RDONLY);
	if (fd < 0)
	{
		perror("File not found.\n");
		exit(0);
	}

	int sockfd;
	struct sockaddr_in serv_addr;

	int i, k;
	char buf[BUFF_SIZE];

	/* Opening a socket is exactly similar to the server process */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Unable to create socket\n");
		exit(0);
	}

	/* Recall that we specified INADDR_ANY when we specified the server
	   address in the server. Since the client can run on a different
	   machine, we must specify the IP address of the server.

	   In this program, we assume that the server is running on the
	   same machine as the client. 127.0.0.1 is a special address
	   for "localhost" (this machine)
	*/

	/*	IF YOUR SERVER RUNS ON SOME OTHER MACHINE, YOU MUST CHANGE
		THE IP ADDRESS SPECIFIED BELOW TO THE IP ADDRESS OF THE
		MACHINE WHERE YOU ARE RUNNING THE SERVER.
	*/

	serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port = htons(PORT);

	/* With the information specified in serv_addr, the connect()
	   system call establishes a connection with the server process.
	*/
	if ((connect(sockfd, (struct sockaddr *)&serv_addr,
				 sizeof(serv_addr))) < 0)
	{
		perror("Unable to connect to server\n");
		exit(0);
	}

	printf("Successfully connected with server.\n");

	/* After connection, the client can send or receive messages.
	   However, please note that recv() will block when the
	   server is not sending and vice versa. Similarly send() will
	   block when the server is not receiving and vice versa. For
	   non-blocking modes, refer to the online man pages.
	*/
	if ((k = recv(sockfd, buf, BUFF_SIZE, 0)) < 0)
	{
		perror("error in recv() in client.\n");
		exit(0);
	}
	buf[k] = '\0';
	printf("%s\n", buf);

	while (1)
	{

		k = read(fd, buf, CHUNK_SIZE);
		if (k < 0)
		{
			perror("read() error\n");
			exit(0);
		}
		if (k == 0)// reached EOF
		{
			// now send a special character,
			// I am taking null, '\0', to say
			// server that we have reached the end
			buf[0]='\0';
			if (send(sockfd, buf, 1, 0) < 0)
			{
				perror("send() error from client.\n");
				exit(0);
			}
			break;
		}

		if (send(sockfd, buf, k, 0) < 0)
		{
			perror("send() error from client.\n");
			exit(0);
		}
	}

	// receive the 3 numbers
	if ((k = recv(sockfd, buf, BUFF_SIZE, 0)) < 0)
	{
		perror("error in recv() in client.\n");
		exit(0);
	}
	buf[k] = '\0';
	printf("Characters : %s\n", buf);

	if ((k = recv(sockfd, buf, BUFF_SIZE, 0)) < 0)
	{
		perror("error in recv() in client.\n");
		exit(0);
	}
	buf[k] = '\0';
	printf("Words : %s\n", buf);

	if ((k = recv(sockfd, buf, BUFF_SIZE, 0)) < 0)
	{
		perror("error in recv() in client.\n");
		exit(0);
	}
	buf[k] = '\0';
	printf("Sentences : %s\n", buf);

	close(sockfd);
	close(fd);
	return 0;
}
