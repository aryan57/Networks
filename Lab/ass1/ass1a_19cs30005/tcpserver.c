/*
	Aryan Agarwal, 19CS30005
	Networks Lab Assgn1
	tcpserver.c
*/
/*
	NETWORK PROGRAMMING WITH SOCKETS

	In this program we illustrate the use of Berkeley sockets for interprocess
	communication across the network. We show the communication between a server
	process and a client process.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* THE SERVER PROCESS */

#define BUFF_SIZE 100
#define CHUNK_SIZE 7
#define PORT 7000
/*
	I intentionally took chunk size of client and server different,
	to show they don't matter. Chunk size of client is not visible to server and vice-versa
*/

int main()
{
	int sockfd, newsockfd; /* Socket descriptors */
	int clilen;
	struct sockaddr_in cli_addr, serv_addr;

	int i, k;
	char buf[BUFF_SIZE]; /* We will use this buffer for communication */

	/* The following system call opens a socket. The first parameter
	   indicates the family of the protocol to be followed. For internet
	   protocols we use AF_INET. For TCP sockets the second parameter
	   is SOCK_STREAM. The third parameter is set to 0 for user
	   applications.
	*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Cannot create socket\n");
		exit(0);
	}

	/* The structure "sockaddr_in" is defined in <netinet/in.h> for the
	   internet family of protocols. This has three main fields. The
	   field "sin_family" specifies the family and is therefore AF_INET
	   for the internet family. The field "sin_addr" specifies the
	   internet address of the server. This field is set to INADDR_ANY
	   for machines having a single IP address. The field "sin_port"
	   specifies the port number of the server.
	*/
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);

	/* With the information provided in serv_addr, we associate the server
	   with its port using the bind() system call.
	*/
	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("Unable to bind local address\n");
		exit(0);
	}

	/*
		This specifies that up to 5 concurrent client
		requests will be queued up while the system is
		executing the "accept" system call below.
	*/
	if (listen(sockfd, 5) < 0)
	{
		perror("Unable to prepare to accept connections on socket FD.\n");
		exit(0);
	}

	printf("TCP Server successfully created.\n");

	/* In this program we are illustrating an iterative server -- one
	   which handles client connections one by one.i.e., no concurrency.
	   The accept() system call returns a new socket descriptor
	   which is used for communication with the server. After the
	   communication is over, the process comes back to wait again on
	   the original socket descriptor.
	*/
	while (1)
	{

		/* The accept() system call accepts a client connection.
		   It blocks the server until a client request comes.

		   The accept() system call fills up the client's details
		   in a struct sockaddr which is passed as a parameter.
		   The length of the structure is noted in clilen. Note
		   that the new socket descriptor returned by the accept()
		   system call is stored in "newsockfd".
		*/
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

		if (newsockfd < 0)
		{
			perror("Accept error from client.\n");
			exit(0);
		}

		printf("Successfully connected with client.\n");

		/* We initialize the buffer, copy the message to it,
			and send the message to the client.
		*/
		strcpy(buf, "Hello from server!!");
		if (send(newsockfd, buf, BUFF_SIZE, 0) < 0)
		{
			perror("error in send() from server.\n");
			exit(0);
		}

		/* We now receive a message from the client. For this example
		   we make an assumption that the entire message sent from the
		   client will come together. In general, this need not be true
		   for TCP sockets (unlike UDPi sockets), and this program may not
		   always work (for this example, the chance is very low as the
		   message is very short. But in general, there has to be some
		   mechanism for the receiving side to know when the entire message
		   is received. Look up the return value of recv() to see how you
		   can do this.
		*/

		/*
			logic:-

			characters = no of characters
			words = count 1 at every start of non-whitespace and non-fullstop character when ended with a whitespace or fullstop
			sentences = no of fullstops

		*/

		int characters = 0, sentences = 0, words = 0;
		int start_of_word = 0;

		while (1)
		{
			k = recv(newsockfd, buf, CHUNK_SIZE, 0);
			if (k < 0)
			{
				perror("recv() error in server.\n");
				exit(0);
			}
			if (k == 0)
				break;

			int to_break=0;
			characters += k;
			for (i = 0; i < k; i++)
			{
				/*
					each text file sent from the client,
					will be ended by a special character by the client
					Here I have used '\0'
				*/
				if(buf[i]=='\0'){
					characters--;
					to_break=1;
					break;
				}

				if (buf[i] == '.')
					sentences++;
				if (buf[i] == ' ' || buf[i] == '\t' || buf[i] == '.' || buf[i] == '\n')
				{
					if (start_of_word != 0)
						words++;
					start_of_word = 0;
				}
				else
				{
					start_of_word = 1;
				}
			}
			if(to_break)break;
		}

		/*
			Now convert these 3 numbers into a string
			and send them one by one
		*/
		k = sprintf(buf, "%d", characters);
		buf[k] = '\0';
		if (send(newsockfd, buf, BUFF_SIZE, 0) < 0)
		{
			perror("error in send() from server.\n");
			exit(0);
		}
		k = sprintf(buf, "%d", words);
		buf[k] = '\0';
		if (send(newsockfd, buf, BUFF_SIZE, 0) < 0)
		{
			perror("error in send() from server.\n");
			exit(0);
		}
		k = sprintf(buf, "%d", sentences);
		buf[k] = '\0';
		if (send(newsockfd, buf, BUFF_SIZE, 0) < 0)
		{
			perror("error in send() from server.\n");
			exit(0);
		}
		close(newsockfd);
		printf("Disconnected from client, waiting for another.\n");
	}
	return 0;
}