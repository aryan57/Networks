/*
	Aryan Agarwal, 19CS30005
	Networks Lab Test1
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
#define MAX_CLIENT 3

/*
	I intentionally took chunk size of client and server different,
	to show they don't matter. Chunk size of client is not visible to server and vice-versa
*/

typedef struct
{
	// both will be stored in network byte order
	struct in_addr ip; // uint32_t 4 bytes
	in_port_t port;	   // uint16_t
} client;

int main()
{

	int sockfd; // Socket descriptors
	struct sockaddr_in serv_addr, cli_addr;
	int cli_len;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Cannot create socket\n");
		exit(EXIT_FAILURE);
	}
	int opt = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		printf("error in setsockopt.\n");
		exit(EXIT_FAILURE);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("Unable to bind local address\n");
		exit(EXIT_FAILURE);
	}
	if (listen(sockfd, 5) < 0)
	{
		perror("Unable to prepare to accept connections on socket FD.\n");
		exit(EXIT_FAILURE);
	}

	// printf("TCP Server successfully created on [PORT : %d] and [address : INADDR_ANY].\n", PORT);

	int clientsockfd[MAX_CLIENT];
	for (int i = 0; i < MAX_CLIENT; i++)
	{
		clientsockfd[i] = -1;
	}

	client clientId[MAX_CLIENT];
	int numclients = 0;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(sockfd, &set);
	for (int i = 0; i < MAX_CLIENT; i++)
	{
		FD_SET(clientsockfd[i], &set);
	}
	char buf[BUFF_SIZE];
	char buf2[BUFF_SIZE];

	while (1)
	{

		// no data about timeout
		// so will wait infinitely
		int k = select(sockfd + 1, &set, NULL, NULL, NULL);
		if (k < 0)
		{
			printf("Error in select.\n");
			exit(EXIT_FAILURE);
		}
		if (k == 0)
		{
			printf("Timeout occured in select.\n");
			continue;
		}

		if (FD_ISSET(sockfd, &set))
		{
			cli_len = sizeof(cli_addr);
			if ((clientsockfd[numclients] = accept(sockfd, (struct sockaddr *)&cli_addr, &cli_len)) < 0)
			{
				perror("Accept error in server\n");
				exit(EXIT_FAILURE);
			}

			// char *cli_ip = inet_ntoa(cli_addr.sin_addr);
			clientId[numclients].ip = cli_addr.sin_addr;
			clientId[numclients].port = cli_addr.sin_port;

			printf("Server: Received a new connection from client <%s: %d>\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

			numclients++;
		}

		for (int i = 0; i < numclients; i++)
		{
			if (FD_ISSET(clientsockfd[i], &set))
			{

				int k = recv(clientsockfd, buf2, BUFF_SIZE, 0);
				if (k < 0)
				{
					perror("recv() error in server.\n");
					exit(0);
				}
				if (k == 0)
					continue;
			}

			printf("Received message \"%s\" from client <%s:%d>\n", buf2, inet_ntoa(clientId[i].ip), ntohs(clientId[i].port));

			if (numclients == 1)
			{
				printf("Insufficient clients, \"%s\" from client <%s:%s> dropped\n", buf2, inet_ntoa(clientId[i].ip), ntohs(clientId[i].port));
				continue;
			}

			// send to client
			int temp = clientId[i].ip.s_addr;
			for (int i = 0; i < 6; i++)
			{
				buf[i] = 0;
			}

			int ind = 3;
			while (temp > 0)
			{
				buf[ind] = temp % 256;
				temp /= 256;
				ind--;
			}

			{
				int ind = 5;
				int temp = clientId[i].port;
				while (temp > 0)
				{
					buf[ind] = temp % 256;
					temp /= 256;
					ind--;
				}
			}

			strcpy(buf + 6, buf2);

			for (int j = 0; j < numclients; j++)
			{
				if (j != i)
				{
					if (send(clientsockfd[j], buf, strlen(buf) + 1, 0) < 0)
					{
						perror("send() error from client.\n");
						exit(0);
					}

					printf("Server: Sent message \"%s\" from client <%s:%d> to <%s:%d>",buf,inet_ntoa(clientId[i].ip),ntohs(clientId[i].port),inet_ntoa(clientId[j].ip),ntohs(clientId[j].port));
				}
			}
		}

		FD_ZERO(&set);
		FD_SET(sockfd, &set);
		for (int i = 0; i < MAX_CLIENT; i++)
		{
			FD_SET(clientsockfd[i], &set);
		}
	}
	return 0;
}