
/*				A NON-BLOCKING CLIENT PROCESS

	Please read the file server.c before you read this file. 

        Normally, if a recv/read is called on a socket in which
        no data is there, the call blocks. In case of a non-blocking
        I/O, the call returns immediately with an error. The client
        can check this error, do other work, and can come back later
        and check again. This program shows a simple example of how
        nonblocking I/O can be done on a socket
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

int main()
{
	int			sockfd ;
	struct sockaddr_in	serv_addr;

	int i, val, err;
	char buf[100];

	/* Opening a socket is exactly similar to the server process */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Unable to create socket\n");
		exit(0);
	}

	/* Recall that we specified 127.0.0.1 when we specified the server address in the server. The client must run on the same machine as the server or you will need to change the IP addresses in client and server appropriately 
	*/
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= inet_addr("127.0.0.1");
	serv_addr.sin_port		= htons(60000);

	/* With the information specified in serv_addr, the connect() system call establishes a connection with the server process.
	*/
        /* Here we will make just a recv() non-blocking, connect can still block if the server is busy (but running)
        */

	while ( connect(sockfd, (struct sockaddr *) &serv_addr,
							sizeof(serv_addr)) < 0) {
           	printf("Unable to connect to server\n");
			exit(0);
	    	  
         }

	/* After connection, the client can send or receive messages. We will make the calls to recv() non-blocking. Recall that the server has a 10 sec. delay in sending after the connection is made. If there is no data to recv, recv will return with -1 and errno will be set to EWOULDBLOCK or EAGAIN. If so, try later.We have just slept here for 1 sec., could have done anything.
	*/

	for(i=0; i < 100; i++) buf[i] = '\0';
	while ( recv(sockfd, buf, 100, MSG_DONTWAIT) < 0) {
           if (errno == EWOULDBLOCK || errno == EAGAIN) {
			printf("No data, recv will block. I can actually do something else \nand come back. Let me sleep anyway\n");
			sleep(1);
            }
	    	else {
                /* some otiher error */
                printf("Fatal error in recv. Exiting\n");
                exit(0);
            }
	}

        /* You are here means data has been received in the buffer */
	printf("%s\n", buf);

	for(i=0; i < 100; i++) buf[i] = '\0';
	strcpy(buf,"Message from client");
	send(sockfd, buf, 100, 0);
	
	close(sockfd);
      return(0);
}

