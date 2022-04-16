/*
			NETWORK PROGRAMMING WITH SOCKETS

In this program we illustrate the use of Berkeley sockets for interprocess
communication across the network. We show the communication between a server
process and a client process.


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>

#define PORT 6969
#define BUF_SIZE 100

void get_ip(char *hostnamebuf, char *ansbuf)
{

	struct hostent *hostinfo = gethostbyname(hostnamebuf);
	if (hostinfo != NULL)
	{
		char **paddrlist = hostinfo->h_addr_list;
		int start = 1;
		while (*paddrlist != NULL)
		{
			char *IPbuffer = inet_ntoa(*((struct in_addr *)*paddrlist));

			if (start == 1)
			{
				strcat(ansbuf, IPbuffer);
				start = 0;
			}
			else
			{
				strcat(ansbuf, ", ");
				strcat(ansbuf, IPbuffer);
			}
			paddrlist++;
		}
	}
	else
	{
		strcat(ansbuf, "0.0.0.0");
	}
}

int main()
{
	struct sockaddr_in cli_addr, serv_addr;
	int tcp_sockfd, tcp_newsockfd, udp_sockfd; /* Socket descriptors */
	int i, cli_len = sizeof(cli_addr);
	char buf[BUF_SIZE]; /* We will use this buffer for communication */
	char hostname[BUF_SIZE];

	/* The following system call opens a socket. The first parameter
	   indicates the family of the protocol to be followed. For internet
	   protocols we use AF_INET. For TCP sockets the second parameter
	   is SOCK_STREAM. The third parameter is set to 0 for user
	   applications.
	*/
	if ((tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Cannot create tcp socket\n");
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
	if (bind(tcp_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("Unable to bind local address\n");
		exit(0);
	}

	listen(tcp_sockfd, 5); /* This specifies that up to 5 concurrent client
				  requests will be queued up while the system is
				  executing the "accept" system call below.
			   */

	if ((udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("Cannot create udp socket\n");
		exit(0);
	}
	if (bind(udp_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("Unable to bind local address\n");
		exit(0);
	}

	fd_set read_file_set;
	FD_ZERO(&read_file_set);

	/* In this program we are illustrating a concurrent server -- one
	   which forks to accept multiple client connections concurrently.
	   As soon as the server accepts a connection from a client, it
	   forks a child which communicates with the client, while the
	   parent becomes free to accept a new connection. To facilitate
	   this, the accept() system call returns a new socket descriptor
	   which can be used by the child. The parent continues with the
	   original socket descriptor.
	*/

	printf("concurrent server running.\n");

	while (1)
	{

		FD_SET(tcp_sockfd, &read_file_set);
		FD_SET(udp_sockfd, &read_file_set);

		if (select((tcp_sockfd > udp_sockfd ? tcp_sockfd : udp_sockfd) + 1, &read_file_set, NULL, NULL, NULL) < 0)
		{
			printf("Error in select\n");
			exit(0);
		}

		if (FD_ISSET(tcp_sockfd, &read_file_set))
		{
			/* The accept() system call accepts a client connection.
			   It blocks the server until a client request comes.

			   The accept() system call fills up the client's details
			   in a struct sockaddr which is passed as a parameter.
			   The length of the structure is noted in clilen. Note
			   that the new socket descriptor returned by the accept()
			   system call is stored in "newsockfd".
			*/
			if ((tcp_newsockfd = accept(tcp_sockfd, (struct sockaddr *)&cli_addr, &cli_len)) < 0)
			{
				perror("Accept error in server\n");
				exit(0);
			}
			/* Having successfully accepted a client connection, the
			   server now forks. The parent closes the new socket
			   descriptor and loops back to accept the next connection.
			*/
			if (fork() == 0)
			{
				/* This child process will now communicate with the
				   client through the send() and recv() system calls.
				*/

				/* 	Close the old socket since all
					communications will be through
					the new socket.
				*/

				close(tcp_sockfd);
				for (i = 0; i < BUF_SIZE; i++)
					buf[i] = '\0';
				if (recv(tcp_newsockfd, buf, BUF_SIZE, 0) < 0)
				{
					printf("Error in TCP recv in server\n");
					exit(0);
				}
				strcpy(hostname, buf);
				for (int i = 0; i < BUF_SIZE; i++)
					buf[i] = '\0';
				get_ip(hostname, buf);
				if (send(tcp_newsockfd, buf, strlen(buf) + 1, 0) < 0)
				{
					perror("tcp send error in server.\n");
					exit(0);
				}
				close(tcp_newsockfd);
				printf("TCP Request processed for %s, waiting for another.\n", hostname);
				exit(0);
			}
			close(tcp_newsockfd);
		}

		if (FD_ISSET(udp_sockfd, &read_file_set))
		{
			for (i = 0; i < BUF_SIZE; i++)
				buf[i] = '\0';
			if (recvfrom(udp_sockfd, (char *)buf, BUF_SIZE, 0, (struct sockaddr *)&cli_addr, &cli_len) < 0)
			{
				perror("recvfrom() error in server.\n");
				exit(0);
			}

			strcpy(hostname, buf);
			for (int i = 0; i < BUF_SIZE; i++)
				buf[i] = '\0';
			get_ip(hostname, buf);
			// sleep(10); // uncomment this to test timeout
			if (sendto(udp_sockfd, (const char *)buf, strlen(buf) + 1, 0, (const struct sockaddr *)&cli_addr, sizeof(cli_addr)) < 0)
			{
				perror("sendto() error in server.\n");
				exit(0);
			}
			printf("UDP Request processed for %s, waiting for another.\n", hostname);
		}
	}
	return 0;
}
