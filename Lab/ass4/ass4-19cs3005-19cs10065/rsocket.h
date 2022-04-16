#ifndef ROCKET_H
#define ROCKET_H

#include <time.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include<errno.h>

#include "pair.h"

#define T 2 /* sleep time, timeout is 2T */
#define P 0.05 /* More P means more dropping of messages */
#define MSG_SIZE 108 /* Max size of the buffer sent using r_sendto */

#define SOCK_MRP SOCK_DGRAM
#define MAX_TABLE_SIZE 50

typedef struct __UNACK_TABLE
{
	char msg[MSG_SIZE]; /*msg with 8 bytes frame at front*/
	int msg_seq_no; /* Message sequence no., -1 represents this entry is empty */
	time_t last_send_time; /* last send time */
	size_t msg_len; /*size of actual msg + 8 bytes framed at front*/

	/*These things are required when we are retransmitting*/
	int fd; /* sockfd of sender */
	int flags; /* The bitwised ORed flags as in UDP sendto calls */
	struct sockaddr addr; /* Reciver's address */
	socklen_t addr_len; /* for storing length of addr */
	
} UNACK_TABLE;

typedef struct __RECV_TABLE
{
	int msg_seq_no; /* Message sequence no., -1 represents this entry is empty */
	size_t msg_len; /*size of actual message*/
	char msg[MSG_SIZE]; /*size of actual message*/
	struct sockaddr addr; /* Receiver's address, so that we can fill address in the r_recvfrom calls */

} RECV_TABLE;



int r_socket(int __domain,int __type,int __protocol);
int r_bind(int __fd,const struct sockaddr * __addr,socklen_t __len);
ssize_t r_sendto(int __fd,const void* __buf,size_t __n,int __flags,const struct sockaddr* __addr,socklen_t __addr_len);
int r_recvfrom(int __fd,void* buf,size_t __n,int __flags,struct sockaddr* __addr,socklen_t* __addr_len);
int r_close(int __fd);

pair getTransmissions();

#endif