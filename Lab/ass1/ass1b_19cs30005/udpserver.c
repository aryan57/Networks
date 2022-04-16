/*
	Aryan Agarwal, 19CS30005
	Networks Lab Assgn1
	udpserver.c
*/
// A Simple UDP Server that sends a HELLO message
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUFF_SIZE 100
#define PORT 7001

int main()
{
	int sockfd;
	struct sockaddr_in servaddr, cliaddr;

	// Create socket file descriptor
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(PORT);

	// Bind the socket with the server address
	if (bind(sockfd,(const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	printf("\nUDP Server Running....\n");

	int i,k;
	socklen_t len;
	char buf[BUFF_SIZE];
	len = sizeof(cliaddr);

	while (1)
	{
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
			k = recvfrom(sockfd, (char *)buf, BUFF_SIZE, 0,( struct sockaddr *) &cliaddr, &len);
			if (k < 0)
			{
				perror("recvfrom() error in server.\n");
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
		k=sprintf(buf, "%d", characters);
		buf[k]='\0';
		if (sendto(sockfd, (const char *)buf, BUFF_SIZE, 0, (const struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0)
		{
			perror("sendto() error in server.\n");
			exit(0);
		}
		k=sprintf(buf, "%d", words);
		buf[k]='\0';
		if (sendto(sockfd, (const char *)buf, BUFF_SIZE, 0, (const struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0)
		{
			perror("sendto() error in server.\n");
			exit(0);
		}
		k=sprintf(buf, "%d", sentences);
		buf[k]='\0';
		if (sendto(sockfd, (const char *)buf, BUFF_SIZE, 0, (const struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0)
		{
			perror("sendto() error in server.\n");
			exit(0);
		}
		printf("Request processed, waiting for another.\n");
	}

	close(sockfd);
	return 0;
}