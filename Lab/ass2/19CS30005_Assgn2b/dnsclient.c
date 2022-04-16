/*
	Aryan Agarwal, 19CS30005
	Networks Lab Assgn2
	dnsclient.c
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

#define PORT 6969
#define BUFF_SIZE 100

int main(int argc, char *argv[])
{

	int sockfd;
	char buf[BUFF_SIZE];
	char hostname[BUFF_SIZE];

	struct sockaddr_in servaddr;

	// Creating socket file descriptor
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	struct timeval tv;
	tv.tv_sec = 2;	// sec
	tv.tv_usec = 0; // microsec
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval)) < 0)
	{
		perror("setting receive timeout failed");
		exit(0);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	// Server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	inet_aton("127.0.0.1", &servaddr.sin_addr);

	strcpy(hostname, "google.com");

	if (sendto(sockfd, (const char *)hostname, strlen(hostname) + 1, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("sendto() error in client.\n");
		exit(0);
	}

	socklen_t len;
	len = sizeof(servaddr);

	if (recvfrom(sockfd, (char *)buf, BUFF_SIZE, 0, (struct sockaddr *)&servaddr, &len) < 0)
	{
		perror("error in recvfrom() in client.\nIt may have timeout.\n");
		exit(0);
	}
	if(buf[0]==0 || strcmp(buf,"0.0.0.0")==0)printf("No valid IPs could be found for %s\n",hostname);
	else printf("IPs for %s : %s\n",hostname, buf);

	close(sockfd);
	return 0;
}