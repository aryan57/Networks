/*
	Networks Lab
	Group 47
	Assignment 6

	Aryan Agarwal, 19CS30005
	Vinit Raj, 19CS10065
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // chdir
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

// #define LINE_BUFFER 200
// #define TOKEN_BUFFER 200
// #define MAX_TOKENS 2
// #define CHUNK_SIZE 6969
#define BUF_SIZE 100
// #define FILE_HEADER_BUF 3
// #define FILE_BUF 200
#define PORT 6969

// char line2[LINE_BUFFER];			  // for sending command (used in mget and mput)
// char token[MAX_TOKENS][TOKEN_BUFFER]; // for parsing tokens
// char buf[BUF_SIZE];
// char file_header[FILE_HEADER_BUF];
// char file_buffer[FILE_BUF];

// int is_open = 0;
// int is_login = 0;
// int n; // will be used for storing length of line(command) entered

int min(int x, int y)
{
	return x < y ? x : y;
}

int main(int argc, char **argv)
{

	int sockfd;
	// Opening a socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("myFTP> Unable to create socket\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port = htons(PORT);

	// establish a connection with the server using the info in serv_addr
	if ((connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
	{
		exit(EXIT_FAILURE);
	}

	char buf[BUF_SIZE];
	int k = 0;
	for (int i = 1; i < argc; i++)
	{
		strcpy(buf, argv[i]);
		if (send(sockfd, buf, BUF_SIZE, 0) < 0)
		{
			perror("send() error from client.\n");
			exit(EXIT_FAILURE);
		}
	}

	if (!strncmp(argv[1], "del", 3))
	{
		if ((k = recv(sockfd, buf, BUF_SIZE, 0)) < 0)
		{
			printf("Error in recv()\n");
			exit(EXIT_FAILURE);
		}
		if (k == 0)
		{
			printf("delete unsuccessfull\n");
		}
		else
		{
			printf("%s\n", buf);
		}
	}
	else if (!strncmp(argv[1], "getbytes", 8))
	{

		int x, y;

		x = atoi(argv[3]);
		y = atoi(argv[4]);
		int remchars = y - x + 1;
		while (remchars--)
		{
			if ((k = recv(sockfd, buf, BUF_SIZE, 0)) < 0)
			{
				printf("Error in recv()\n");
				exit(EXIT_FAILURE);
			}
			if (k == 0)
			{
				break;
			}
			else
			{
				printf("%c", buf[0]);
			}
		}
	}

	close(sockfd);

	return 0;
}
