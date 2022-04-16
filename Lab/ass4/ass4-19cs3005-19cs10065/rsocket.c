#include "rsocket.h"

pthread_t R,S;
pthread_mutex_t mutex;
int sockfd;
UNACK_TABLE unack_table[MAX_TABLE_SIZE];
RECV_TABLE recv_table [MAX_TABLE_SIZE];
int msg_cntr = 0;
int transmissions=0;

#define DEBUG 0 /* If 1 then debugging messages will be printed */

/*
	Returns the index of the UNACK_TABLE
	where entry can be filled or -1 if UNACK_TABLE
	is full
*/
int empty_position_unack_table(){
	for(int i=0;i<MAX_TABLE_SIZE;i++){
		if(unack_table[i].msg_seq_no==-1)return i;
	}
	return -1;
}
/*
	Returns the index of the UNACK_TABLE
	where entry is filled or -1 if UNACK_TABLE
	is empty
*/
int filled_position_unack_table(){
	for(int i=0;i<MAX_TABLE_SIZE;i++){
		if(unack_table[i].msg_seq_no!=-1)return i;
	}
	return -1;
}
/*
	Returns the first index of the RECV_TABLE
	where entry is present or -1 if RECV_TABLE
	is empty
*/
int filled_position_recv_table(){
	for(int i=0;i<MAX_TABLE_SIZE;i++){
		if(recv_table[i].msg_seq_no!=-1)return i;
	}
	return -1;
}

/*
	add the 8 byte frame to the SRC_BUF.
	Returns the sizeof DEST_BUF on success
	or -1 if the new buffer size exceeds MSG_SIZE

	First 4 bytes represents id of msg.
	Next 4 bytes represents type of msg, 1 for ACK and 0 for data.
	Last 100 bytes represents the message.

*/
int add_frame_8_byte(const void* __src_buf,void** __dest_buf,size_t len,int __frame2){

	++msg_cntr;

	if(len+2*(sizeof(int))>MSG_SIZE){
		__dest_buf = NULL;
		return -1;
	}

	*__dest_buf = calloc(1,2*sizeof(int)+len);
	memcpy(*__dest_buf,(void*)&msg_cntr,sizeof(int));
	memcpy(*__dest_buf+sizeof(int),(void*)&__frame2,sizeof(int));
	memcpy(*__dest_buf+2*sizeof(int),__src_buf,len);

	/*
		donrt return sizeof(__dest_buf), as it
		will return the size of the pointer
		which will be 8 on a 64 bit system
	*/
	return 2*sizeof(int)+len;
}

/*
	This function first generates a random number
	between 0 and 1. If the generated number is < p,
	then the function returns 1, else it returns 0
*/
int dropMessage(float p) {
	float rand_0_1 = (float)rand() / RAND_MAX;
    return (rand_0_1 < p);
}

/*
	Handles the thread R. It waits for a message to come in a recvfrom() call.
	When it receives a message, if it is a data message, it stores it in the
	received-message table, and sends an ACK message to the sender. If it is an ACK message
	in response to a previously sent message, it updates the unacknowledged-message table to
	take out the message for which the acknowledgement has arrived.
*/
void* R_handler( void* arg){
	while (1)
	{
		/*
			Not starting the lock from here
			as recvfrom() is a blocking call
			so the R thread will keep running
			and wont receive anything
			as initially nothing is sent
		*/
		char buf[MSG_SIZE];
		struct sockaddr_in recv_adddr;
		socklen_t len = sizeof(recv_adddr);
		
		int k = recvfrom(sockfd,buf,MSG_SIZE,0,(struct sockaddr*)&recv_adddr,&len);
		if(k<0 || k-2*sizeof(int)<0 || errno==EWOULDBLOCK || errno==EAGAIN){
			continue;
		}
		pthread_mutex_lock(&mutex);
		int msg_seq_no = 0;
		memcpy(&msg_seq_no,buf,sizeof(int));
		int is_ack=0;
		memcpy(&is_ack,buf+sizeof(int),sizeof(int));
		if(dropMessage(P)){
			/*
				Drop the message by simply doing nothing
			*/
		if(DEBUG!=0){
			printf("Message dropped for [msg_seq_no : %d] [buf : %s]\n",msg_seq_no,buf+8);
		}
			pthread_mutex_unlock(&mutex);
			continue;
		}

		if(is_ack==1){
			if(DEBUG!=0){
				printf("Ack received for [msg_seq_no : %d] [buf : %s]\n",msg_seq_no,buf+8);
			}
			/*
				Its an ack message
				so removed entry from unack_table
				which matches with msg_seq_no
			*/
			for(int i=0;i<MAX_TABLE_SIZE;i++){
				if(unack_table[i].msg_seq_no==msg_seq_no){
					unack_table[i].msg_seq_no=-1;
					break;
				}
			}
		}else{
			if(DEBUG!=0){
				printf("Data message received for [msg_seq_no : %d] [buf : %s]\n",msg_seq_no,buf+8);
			}
			/*
				Its an data message
				store the entry in recv_table
				and send the ack
			*/
			for(int i=0;i<MAX_TABLE_SIZE;i++){
				if(recv_table[i].msg_seq_no==-1){
					recv_table[i].msg_seq_no=msg_seq_no;
					recv_table[i].addr=*((struct sockaddr*)&recv_adddr);
					recv_table[i].msg_len=k-2*sizeof(int);
					memcpy(recv_table[i].msg,buf+2*sizeof(int),recv_table[i].msg_len);
					break;
				}
			}
			int ack=1;
			memcpy(buf+sizeof(int),(void*)&ack,sizeof(int));
			if(DEBUG!=0){
				printf("Sending ack for [msg_seq_no : %d] [buf : %s]\n",msg_seq_no,buf+8);
			}
			sendto(sockfd,buf,k,0,(const struct sockaddr*)&recv_adddr,len);
			transmissions++; // increasing transmissions counter
		}

		pthread_mutex_unlock(&mutex);
	}
	
	return NULL;
}

/*
	Handles the thread S
	It sleeps for time (T), and wakes up periodically.
	On waking up, it scans the unacknowledged-message table to see if any of
	the messages timeout period (set to 2T ) is over (from the difference between the time in
	the table entry for a message and the current time). If yes, it retransmits that message and
	resets the time in that entry of the table to the new sending time. If not, no action is taken.
	This is repeated for all messages in the table every time S wakes
*/
void* S_handler(void* arg){
	while (1)
	{
		sleep(T);
		pthread_mutex_lock(&mutex);
		for(int i=0;i<MAX_TABLE_SIZE;i++){
			if(unack_table[i].msg_seq_no==-1)continue;
			time_t cur_time = time(NULL);
			if(cur_time-unack_table[i].last_send_time>=2*T){
				
				if(DEBUG!=0){
					char* temp = (char*)unack_table[i].msg;
					printf("Retransmitting [msg_seq_no : %d] [buf : %s]\n",unack_table[i].msg_seq_no,temp+8);
				}

				unack_table[i].last_send_time=cur_time;
				sendto(unack_table[i].fd,unack_table[i].msg,unack_table[i].msg_len,unack_table[i].flags,&unack_table[i].addr,unack_table[i].addr_len);
				transmissions++; // increasing transmissions counter
			}
		}
		pthread_mutex_unlock(&mutex);
	}
	
	return NULL;
}
/*
	Create a new socket of type TYPE in domain DOMAIN, using
	protocol PROTOCOL. If PROTOCOL is zero, one is chosen automatically.
	Returns a file descriptor for the new socket, or -1 for errors.

	This function opens an UDP socket with the socket call. It also creates the 2
	threads R and S, and allocates initial space for the tables. The parameters to these are
	the same as the normal socket( ) call, except that it will take only SOCK_MRP as the
	socket type.
*/
int r_socket(int __domain,int __type,int __protocol){

	/*
		initialse the tables
	*/
	for(int i=0;i<MAX_TABLE_SIZE;i++){
		unack_table[i].msg_seq_no=-1;
		bzero(unack_table[i].msg,MSG_SIZE);
		
		recv_table[i].msg_seq_no=-1;
		bzero(recv_table[i].msg,MSG_SIZE);
	}

	sockfd = socket(__domain,__type,__protocol);
	if(sockfd<0){
		return -1;
	}

	int res=0;
	/*
		Both pthread_mutex_init and pthread_create on success
		returns 0, else returns the error number
	*/
	res=pthread_mutex_init(&mutex,NULL);
	if(res!=0)return -1;
	res=pthread_create(&R,NULL,R_handler,NULL);
	if(res!=0)return -1;
	res=pthread_create(&S,NULL,S_handler,NULL);
	if(res!=0)return -1;

	return sockfd;
}

/*
	Give the socket FD the local address ADDR (which is LEN bytes long).
	On success, zero is returned.  On error, -1 is returned, and errno is set appropriately.

	Binds the socket with some address-port using the bind call.
*/
int r_bind(int __fd,const struct sockaddr * __addr,socklen_t __len){
	return bind(__fd,__addr,__len);
}


/*
	Send N bytes of BUF on socket FD to peer at address ADDR (which is
	ADDR_LEN bytes long). Returns the number sent, or -1 for errors.

	sends the message immediately by sendto. It also adds a message sequence
	no. at the beginning of the message and stores the message along with its sequence no.
	and destination address-port in the unacknowledged-message table before sending the
	message. With each entry, there is also a time field that is filled up initially with the
	time of first sending the message.
*/
ssize_t r_sendto(int __fd,const void* __buf,size_t __n,int __flags,const struct sockaddr* __addr,socklen_t __addr_len){
	pthread_mutex_lock(&mutex);
	int position = empty_position_unack_table();
	if(position<0){
		pthread_mutex_unlock(&mutex);
		return -1;
	}
	void *buf_with_frame = NULL;
	__n = add_frame_8_byte(__buf,&buf_with_frame,__n,0); /*0 means not an ack*/
	if(__n  < 0){
		pthread_mutex_unlock(&mutex);
		return -1;
	}

	unack_table[position].msg_seq_no=msg_cntr;
	unack_table[position].msg_len=__n;
	memcpy(unack_table[position].msg,buf_with_frame,__n);
	free(buf_with_frame);


	/*saving these states so that we can do retransmitting*/
	unack_table[position].addr=*__addr;
	unack_table[position].addr_len=__addr_len;
	unack_table[position].fd=__fd;
	unack_table[position].flags=__flags;

	unack_table[position].last_send_time=time(NULL); /* means current time */
	ssize_t res = sendto(__fd,unack_table[position].msg,__n,__flags,__addr,__addr_len);
	if(res<0){
		/*
			UDP send was unsuccessful,
			so removing this entry from
			unack_table
		*/
		unack_table[position].msg_seq_no = -1;
	}
	transmissions++; // increasing transmissions counter
	pthread_mutex_unlock(&mutex);
	return res;
}



/*
	Read N bytes into BUF through socket FD.
	If ADDR is not NULL, fill in *ADDR_LEN bytes of it with tha address of
	the sender, and store the actual size of the address in *ADDR_LEN.
	Returns the number of bytes read or -1 for errors.

	looks up the received-message table to see if any message is already
	received in the underlying UDP socket or not. If yes, it returns the first message and
	deletes that message from the table. If not, it blocks the call. To block the r_recvfrom
	call, you can use sleep call to wait for some time and then see again if a message is
	received. r_recvfrom, similar to recvfrom, is a blocking call by default and returns to
	the user only when a message is available.
*/
int r_recvfrom(int __fd,void* buf,size_t __n,int __flags,struct sockaddr* __addr,socklen_t* __addr_len){
	
	/* __flags is not used here */
	while (1)
	{
		pthread_mutex_lock(&mutex);
		int position = filled_position_recv_table();
		if(position==-1){
			/*No entry found , sleep*/
			pthread_mutex_unlock(&mutex);
			sleep(T);
		}else{
			if(__addr!=NULL){
				*__addr_len=sizeof(recv_table[position].addr);
				memcpy(__addr,&recv_table[position].addr,*__addr_len);
			}
			if(__n>recv_table[position].msg_len){
				/*
					__n is the max bytes __buf can take
					but we have only recv_table[position].msg_len
					so will copy whats available
				*/
				__n = recv_table[position].msg_len;
			}
			memcpy(buf,recv_table[position].msg,__n);
			recv_table[position].msg_seq_no=-1; /* delete entry from recv table */
			pthread_mutex_unlock(&mutex);
			return __n;
		}
	}
	return 0; /*Unreachable point*/
}

/*
	Close the file descriptor FD.

	closes the socket; kills all threads and frees all memory associated with the
	socket. If any data is there in the received-message table, it is discarded
*/
int r_close(int __fd){
	/*
		tables were not made using malloc,
		so need of freeing them
	*/
	if(pthread_kill(R,SIGKILL)<0){
		return -1;
	}
	if(pthread_kill(S,SIGKILL)<0){
		return -1;
	}
	return close(__fd);
}

/*
	Returns a pair of integers with the first one being the
	total number of transmissions including the retransmits
	for sending the characters and second one being the position
	of first filled entry in the unack table or -1 if unack table
	is empty
*/
pair getTransmissions(){
	
	pair answer;
	answer.first=transmissions;
	answer.second=filled_position_unack_table();
	return answer;
}