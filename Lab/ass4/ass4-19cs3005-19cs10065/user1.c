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
#include "pair.h"

#define PORT1 50000 + 2 * (0005)
#define PORT2 50000 + 2 * (0005) + 1
#define MAX_BUF_SIZE 100

int main()
{
	int M1 = r_socket(AF_INET,SOCK_MRP, 0);
	if (M1 < 0)
	{
		printf("error in socket creation.\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in addr1,addr2;

	memset(&addr1, 0, sizeof(addr1));
	memset(&addr2, 0, sizeof(addr2));

	// user1 information
	addr1.sin_family = AF_INET;
	addr1.sin_port = htons(PORT1);
	inet_aton("127.0.0.1", &addr1.sin_addr);

	// user2 information
	addr2.sin_family = AF_INET;
	addr2.sin_port = htons(PORT2);
	inet_aton("127.0.0.1", &addr2.sin_addr);

	// bind user1 address
	if (r_bind(M1, (struct sockaddr *)&addr1, sizeof(addr1)) < 0)
	{
		printf("Error in bind.\n");
		exit(EXIT_FAILURE);
	}

	char buf[MAX_BUF_SIZE];
	printf("Please enter the string you want to send : ");
	scanf("%s",buf);
	printf("\n");
	fflush(stdout);

	for(int i=0;i<strlen(buf);i++){
		// send each character to user2
		if (r_sendto(M1, buf+i, 1, 0, (const struct sockaddr *)&addr2, sizeof(addr2)) < 0)
		{
			printf("error in send.\n");
			exit(EXIT_FAILURE);
		}
	}

	int transmissions = -1;
	while (1)
	{
		pair answer = getTransmissions();
		if(answer.second==-1){
			transmissions=answer.first;
			break;
		}
	}
	
	printf("Transmissions required to send [%d characters] with [%4.2f drop probabilty] : %d\n",(int)strlen(buf),P,transmissions);
	printf("Average number of transmissions made to send each character : %4.2lf\n",(double)(transmissions)/(double)strlen(buf));
	while (1); // run indefinitely, dont call close
	return 0;
}