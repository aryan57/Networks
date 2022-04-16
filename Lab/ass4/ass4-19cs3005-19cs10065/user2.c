#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>

#include "rsocket.h"

#define PORT2 50000 + 2 * (0005) + 1
#define MAX_BUF_SIZE 100

int main()
{
	int M2 = r_socket(AF_INET,SOCK_MRP, 0);
	if (M2 < 0)
	{
		printf("error in socket creation.\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in addr2;

	memset(&addr2, 0, sizeof(addr2));

	// user2 information
	addr2.sin_family = AF_INET;
	addr2.sin_port = htons(PORT2);
	inet_aton("127.0.0.1", &addr2.sin_addr);

	// bind user2 address
	if (r_bind(M2, (struct sockaddr *)&addr2, sizeof(addr2)) < 0)
	{
		printf("Error in bind.\n");
		exit(EXIT_FAILURE);
	}

	while (1)
	{

		char buf[MAX_BUF_SIZE];
		bzero(buf,MAX_BUF_SIZE);
		int k=r_recvfrom(M2, buf, MAX_BUF_SIZE, 0, NULL, NULL);
		if (k < 0)
		{
			printf("error in r_recvfrom\n");
			exit(EXIT_FAILURE);
		}
		if(k==0)break;
		printf("%s",buf);
		fflush(stdout);
	}
	
	return 0;
}