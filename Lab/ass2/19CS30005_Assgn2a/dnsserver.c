/*
	Aryan Agarwal, 19CS30005
	Networks Lab Assgn2
	dnsserver.c
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
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include<string.h>

#define BUFF_SIZE 100
#define PORT 7001

void get_ip(char *hostnamebuf,char *ansbuf){
	
	struct hostent *hostinfo = gethostbyname(hostnamebuf);
	if (hostinfo != NULL)
	{
		char **paddrlist = hostinfo->h_addr_list;
		int start=1;
		while (*paddrlist != NULL)
		{
			char *IPbuffer = inet_ntoa(*((struct in_addr *)*paddrlist));

			if(start==1){
				strcat(ansbuf,IPbuffer);
				start=0;
			}
			else{
				strcat(ansbuf,", ");
				strcat(ansbuf,IPbuffer);
			}
			paddrlist++;
		}
	}
	else
	{
		strcat(ansbuf,"0.0.0.0");
	}
}

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

	socklen_t len;
	char buf[BUFF_SIZE];
	char hostname[BUFF_SIZE];
	len = sizeof(cliaddr);

	while (1)
	{
		if (recvfrom(sockfd, (char *)buf, BUFF_SIZE, 0,( struct sockaddr *) &cliaddr, &len) < 0)
		{
			perror("recvfrom() error in server.\n");
			exit(0);
		}

		strcpy(hostname,buf);
		for(int i=0;i<BUFF_SIZE;i++)buf[i]='\0';
		get_ip(hostname,buf);
		// sleep(10); // uncomment this to test timeout
		if (sendto(sockfd, (const char *)buf, strlen(buf)+1, 0, (const struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0)
		{
			perror("sendto() error in server.\n");
			exit(0);
		}
		printf("Request processed for %s, waiting for another.\n",hostname);
	}

	close(sockfd);
	return 0;
}