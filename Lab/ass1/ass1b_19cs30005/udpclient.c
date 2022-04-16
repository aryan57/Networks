/*
	Aryan Agarwal, 19CS30005
	Networks Lab Assgn1
	udpclient.c
*/
// A Simple Client Implementation
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>

#define PORT 7001
#define BUFF_SIZE 100
#define CHUNK_SIZE 10

int main(int argc, char *argv[])
{
	/*
		since the file name has to be given from command line,
		there must be atleast 2 arguments (everything after 2nd argument will be ignored).
		Example: ./udpclient foo.txt
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

	int sockfd, i, k;
	char buf[BUFF_SIZE];
	struct sockaddr_in servaddr;

	// Creating socket file descriptor
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));

	// Server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	inet_aton("127.0.0.1", &servaddr.sin_addr);

	while (1)
	{

		k = read(fd, buf, CHUNK_SIZE);
		if (k < 0)
		{
			perror("read() error\n");
			exit(0);
		}
		if (k == 0)
		{
			// now send a special character,
			// I am taking null, '\0', to say
			// server that we have reached the end
			buf[0]='\0';
			if (sendto(sockfd, (const char *)buf, 1, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr))< 0)
			{
				perror("send() error from client.\n");
				exit(0);
			}
			break;
		} // reached EOF

		buf[k]='\0';
		if (sendto(sockfd, (const char *)buf, k, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		{
			perror("sendto() error in client.\n");
			exit(0);
		}
	}

	// receive the 3 numbers
	socklen_t len;
	len = sizeof(servaddr);

	if ((k = recvfrom(sockfd, (char *)buf, BUFF_SIZE, 0, (struct sockaddr *)&servaddr, &len)) < 0)
	{
		perror("error in recvfrom() in client.\n");
		exit(0);
	}

	buf[k] = '\0';
	printf("Characters : %s\n", buf);

	if ((k = recvfrom(sockfd, (char *)buf, BUFF_SIZE, 0, (struct sockaddr *)&servaddr, &len)) < 0)
	{
		perror("error in recvfrom() in client.\n");
		exit(0);
	}
	buf[k] = '\0';
	printf("Words : %s\n", buf);

	if ((k = recvfrom(sockfd, (char *)buf, BUFF_SIZE, 0, (struct sockaddr *)&servaddr, &len)) < 0)
	{
		perror("error in recvfrom() in client.\n");
		exit(0);
	}
	buf[k] = '\0';
	printf("Sentences : %s\n", buf);

	close(sockfd);
	close(fd);
	return 0;
}