/*
	Networks Lab Test

	Aryan Agarwal, 19CS30005
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 100
// #define COMMAND_BUFFER 200
// #define RESULT_BUFFER 200
// #define USER_BUFFER 200
// #define FILE_BUF 200
// #define FILE_HEADER_BUF 3 // 1 byte for 'M' or 'L' and 16 bits for length of frame
#define PORT 6969
#define CHUNK_SIZE 69

// char command[COMMAND_BUFFER]; // We will use this buffer for receiving commands
// char result[RESULT_BUFFER];	  // We will use this buffer for sending results
// char buf[BUF_SIZE];			  // we will use this buffer for reading files
// char user[USER_BUFFER];		  // we will use this buffer for reading user
// char file_buffer[FILE_BUF];
// char file_header[FILE_HEADER_BUF];

void *handle_thread(void *ptr)
{
	int newsockfd = *((int *)ptr);
	char buf[BUF_SIZE];

	int k;
	if ((k = recv(newsockfd, buf, BUF_SIZE, 0)) <= 0)
	{
		printf("[server] Error in recv()\n");
		close(newsockfd);
		pthread_exit(NULL);
		return NULL;
	}

	if (!strncmp(buf, "del", 3))
	{

		if ((k = recv(newsockfd, buf, BUF_SIZE, 0)) <= 0)
		{
			printf("[server] Error in recv()\n");
			close(newsockfd);
			pthread_exit(NULL);
			return NULL;
		}

		if (remove(buf) == 0)
		{
			strcpy(buf, "delete success");
			if ((k = send(newsockfd, buf, BUF_SIZE, 0)) <= 0)
			{
				printf("[server] Error in send()\n");
				close(newsockfd);
				pthread_exit(NULL);
				return NULL;
			}
		}
		else
		{
			close(newsockfd);
			pthread_exit(NULL);
			return NULL;
		}
	}
	else if (!strncmp(buf, "getbytes", 8))
	{
		if ((k = recv(newsockfd, buf, BUF_SIZE, 0)) <= 0)
		{
			printf("[server] Error in recv()\n");
			close(newsockfd);
			pthread_exit(NULL);
			return NULL;
		}
		char filename[3*BUF_SIZE];
		strcpy(filename, buf);
		int x = 0, y = 0;
		if ((k = recv(newsockfd, buf, BUF_SIZE, 0)) <= 0)
		{
			printf("[server] Error in recv()\n");
			close(newsockfd);
			pthread_exit(NULL);
			return NULL;
		}
		x = atoi(buf);
		if ((k = recv(newsockfd, buf, BUF_SIZE, 0)) <= 0)
		{
			printf("[server] Error in recv()\n");
			close(newsockfd);
			pthread_exit(NULL);
			return NULL;
		}
		y = atoi(buf);

		int fd = open(filename, O_RDONLY);
		if (fd < 0)
		{
			close(newsockfd);
			pthread_exit(NULL);
			return NULL;
		}
		int file_size = 0;
		char file_buffer[BUF_SIZE];

		// printf("%d %d %s\n",x,y,filename);

		while (1)
		{
			if ((k = read(fd, file_buffer, BUF_SIZE)) < 0)
			{
				printf("error in reading file.\n");
				close(newsockfd);
				pthread_exit(NULL);
				return NULL;
			}
			if (k == 0)
				break;
			file_size += k;
		}
		if (x >= file_size || x < 0 || y >= file_size || y < 0 || x > y)
		{
			close(newsockfd);
			pthread_exit(NULL);
			return NULL;
		}

		close(fd);
		fd = open(filename, O_RDONLY);
		// printf("%d %d %s *\n",x,y,filename);
		

		int read_so_far=0;
		while (1)
		{
			if ((k = read(fd, file_buffer, BUF_SIZE)) < 0)
			{
				printf("error in reading file.\n");
				close(newsockfd);
				pthread_exit(NULL);
				return NULL;
			}
			if (k == 0)
				break;

			int st_ind=read_so_far;
			int en_ind=read_so_far+k-1;
				// printf("%s * \n",file_buffer);
				// printf("%d %d * \n",st_ind,en_ind);

			for(int ind=st_ind;ind<=en_ind;ind++){
				if(ind<x || ind>y)continue;
				// printf("%c * \n",file_buffer[ind-st_ind]);
				if (send(newsockfd, file_buffer+ind-st_ind, BUF_SIZE,0) <= 0)
				{
					printf("error in reading file.\n");
					close(newsockfd);
					pthread_exit(NULL);
					return NULL;
				}
			}

			read_so_far += k;

		}

		printf("byte <%d> to byte <%d> of file <%s> sent\n",x,y,filename);
		close(newsockfd);
		close(fd);
		pthread_exit(NULL);
		return NULL;
	}
}

int main()
{
	int sockfd; // Socket descriptors
	struct sockaddr_in serv_addr;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Cannot create socket\n");
		exit(0);
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
		exit(0);
	}
	if (listen(sockfd, 5) < 0)
	{
		perror("Unable to prepare to accept connections on socket FD.\n");
		exit(0);
	}

	printf("TCP Server successfully created on [PORT : %d] and [address : INADDR_ANY].\n", PORT);

	while (1)
	{
		int newsockfd;
		struct sockaddr_in cli_addr;
		int clilen = sizeof(cli_addr);
		if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) < 0)
		{
			// perror("Accept error from client.\n");
			// exit(EXIT_FAILURE);
			continue;
		}

		printf("Successfully connected with client.\n");

		/*
			Having successfully accepted a client connection, the
			server now forks. The parent closes the new socket
			descriptor and loops back to accept the next connection.
		*/

		pthread_t thread;
		pthread_create(&thread, NULL, &handle_thread, (void *)&newsockfd);
	}

	close(sockfd);
	return 0;
}