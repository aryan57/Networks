/*
	Networks Lab
	Group 47
	Assignment 6
	
	Aryan Agarwal, 19CS30005
	Vinit Raj, 19CS10065
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // chdir
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

#define LINE_BUFFER 200
#define TOKEN_BUFFER 200
#define MAX_TOKENS 2
#define CHUNK_SIZE 6969
#define BUF_SIZE 200
#define FILE_HEADER_BUF 3
#define FILE_BUF 200

char line[LINE_BUFFER]; // for sending command
char line2[LINE_BUFFER]; // for sending command (used in mget and mput)
char token[MAX_TOKENS][TOKEN_BUFFER]; // for parsing tokens
char buf[BUF_SIZE];
char file_header[FILE_HEADER_BUF];
char file_buffer[FILE_BUF];

int is_open = 0;
int is_login=0;
int sockfd;
int n; // will be used for storing length of line(command) entered
struct sockaddr_in serv_addr;

void do_send_recv()
{
	if (send(sockfd, line, n + 1, 0) < 0)
	{
		printf("myFTP> error in send() from the client.\n");
		exit(EXIT_FAILURE);
	}
	if (recv(sockfd, buf, BUF_SIZE, 0) < 0)
	{
		printf("myFTP> [client] Error in recv()\n");
		exit(EXIT_FAILURE);
	}
}

int min(int x, int y)
{
	return x < y ? x : y;
}

// store 16 bit binary representaion 
// of d in file_header[]
void calc_binary(int d)
{
	// int ind = 16;
	// while (ind > 0)
	// {
	// 	if (d % 2)
	// 		file_header[ind--] = '1';
	// 	else
	// 		file_header[ind--] = '0';
	// 	d /= 2;
	// }

	int mask = (1<<8) - 1;
	file_header[2]=mask&d;
	mask<<=8;
	file_header[1]=mask&d;
}

void handle_open()
{
	if (is_open)
	{
		printf("myFTP> connection is already opened with the server.\n");
		return;
	}
	int index = 0;
	char *ptr = strtok(line, " ");
	ptr = strtok(NULL, " ");
	while (ptr != NULL && index < 2 && index < MAX_TOKENS)
	{
		strcpy(token[index++], ptr);
		ptr = strtok(NULL, " ");
	}

	// Opening a socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("myFTP> Unable to create socket\n");
		exit(EXIT_FAILURE);
	}

	// token[0] = IP
	// token[1] = address_port

	serv_addr.sin_family = AF_INET;
	inet_aton(token[0], &serv_addr.sin_addr);
	serv_addr.sin_port = htons(atoi(token[1]));

	// establish a connection with the server using the info in serv_addr
	if ((connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
	{
		printf("myFTP> Unable to connect to server on [IP:%s] at [PORT:%s]\n", token[0], token[1]);
		return;
		// exit(EXIT_FAILURE);
	}

	printf("myFTP> Successfully connected with server\n");
	is_open = 1;
}

void handle_user()
{
	if (!is_open)
	{
		printf("myFTP> 'open IP port' must be the first command\n");
		return;
	}
	// if (is_login)
	// {
	// 	printf("myFTP> user already login, please proceed.\n");
	// 	return;
	// }

	// do send and receive calls
	do_send_recv();

	if (!strcmp(buf, "200"))
	{
		printf("myFTP> [Response Code:%s] Command executed successfully\n", buf);
	}
	else
	{
		printf("myFTP> [Response Code:%s] Error executing command.\n", buf);
	}
}

void handle_pass()
{
	if (!is_open)
	{
		printf("myFTP> 'open IP port' must be the first command\n");
		return;
	}
	// if (is_login)
	// {
	// 	printf("myFTP> user already login, please proceed.\n");
	// 	return;
	// }

	// do send and receive calls
	do_send_recv();

	if (!strcmp(buf, "200"))
	{
		is_login = 1;
		printf("myFTP> [Response Code:%s] Command executed successfully\n", buf);
	}
	else
	{
		printf("myFTP> [Response Code:%s] Error executing command.\n", buf);
	}
}

void handle_lcd()
{
	if (!is_open)
	{
		printf("myFTP> 'open IP port' must be the first command\n");
		return;
	}
	if (!is_login)
	{
		printf("myFTP> user not login, please login first.\n");
		return;
	}
	char *ptr = strtok(line, " ");
	ptr = strtok(NULL, " ");
	if (chdir(ptr) < 0)
	{
		printf("myFTP> [client] Error : cant change the local directory\n");
	}
	else
		printf("myFTP> new local directory is '%s'\n", ptr);
}

void handle_quit()
{
	close(sockfd);
	exit(EXIT_SUCCESS);
}

void handle_cd()
{
	if (!is_open)
	{
		printf("myFTP> 'open IP port' must be the first command\n");
		return;
	}
	// if (!is_login)
	// {
	// 	printf("myFTP> user not login, please login first.\n");
	// 	return;
	// }
	do_send_recv();
	if (!strcmp(buf, "200"))
	{
		printf("myFTP> [Response Code:%s] Command executed successfully\n", buf);
	}
	else
	{
		printf("myFTP> [Response Code:%s] Error executing command\n", buf);
	}
}

void handle_dir()
{
	if (!is_open)
	{
		printf("myFTP> 'open IP port' must be the first command\n");
		return;
	}
	// if (!is_login)
	// {
	// 	printf("myFTP> user not login, please login first.\n");
	// 	return;
	// }

	if (send(sockfd, line, n + 1, 0) < 0)
	{
		printf("myFTP> error in send() from the client.\n");
		exit(EXIT_FAILURE);
	}

	int can_ignore = 0;
	char response_code[4] = {};
	int index = 0;
	int file_no = 0;
	char prevc = '\0';
	int k;
	while (1)
	{
		if ((k = recv(sockfd, buf, CHUNK_SIZE, 0)) < 0)
		{
			printf("recv() error in client.\n");
			exit(EXIT_FAILURE);
		}

		if (k == 0)
			break;
		int can_break = 0;
		for (int i = 0; i < k; i++, index++)
		{

			if (index < 3)
				response_code[index] = buf[i];

			if (buf[i] == '\0')
			{
				if (!can_ignore)
				{
					can_break = 1;
					break;
				}
				can_ignore = 0;
				file_no++;
				if (index == 3)
				{
					if (!strcmp("200", response_code))
					{
						printf("myFTP> [Response Code:%s] Command executed successfully\n", response_code);
					}
					else
					{
						printf("myFTP> [Response Code:%s] Error executing command\n", response_code);
						can_break = 1;
						break;
					}
				}
				else
				{
					printf("\n");
				}
			}
			else
			{
				can_ignore = 1;
				if (index >= 4)
				{
					if (prevc == '\0')
						printf("myFTP> [%d] ", file_no);
					printf("%c", buf[i]);
				}
			}
			prevc = buf[i];
		}
		if (can_break)
			break;
	}
}

void handle_ldir()
{
	if (!is_open)
	{
		printf("myFTP> 'open IP port' must be the first command\n");
		return;
	}
	if (!is_login)
	{
		printf("myFTP> user not login, please login first.\n");
		return;
	}

	struct dirent *directory_entry;
	DIR *dr = opendir(".");
	if (dr == NULL)
	{
		// Could not open current directory
		printf("[client] Error in send()\n");
		exit(EXIT_FAILURE);
	}

	int file_no = 0;
	while ((directory_entry = readdir(dr)) != NULL)
	{
		printf("myFTP> [%d] %s\n", ++file_no, directory_entry->d_name);
	}
	closedir(dr);
}

void handle_get_each(char* remote_file,char* local_file,int *ok){
	int fd = open(local_file, O_TRUNC | O_CREAT | O_WRONLY,0644);

	/*
		O_TRUNC if file exists truncate it 0, as we want to overwrite
		O_CREAT create file if not exist in the specified path
		O_WRONLY only want write access
	*/
	// token[1][0]=0;
	// token[0][0]=0;
	if (fd < 0)
	{
		printf("myFTP> can't open [localfile : %s] for writing\n", token[1]);
		return;
	}

	// constructin the get command
	strcpy(line2, "get ");
	strcat(line2, remote_file);
	strcat(line2, " ");
	strcat(line2, local_file);
	n = strlen(line2);

	if (send(sockfd, line2, n + 1, 0) < 0)
	{
		printf("myFTP> error in send() from the client.\n");
		exit(EXIT_FAILURE);
	}

	// now receive
	// first receive 4 bytes to check response code
	if (recv(sockfd, buf, 4, MSG_WAITALL) < 0)
	{
		printf("myFTP> [client] Error in recv() for response code\n");
		exit(EXIT_FAILURE);
	}

	if (!strcmp(buf, "200"))
	{

		// printf("myFTP> [Response Code:%s] Command executed successfully\n", buf);
		int to_break = 0;
		while (!to_break)
		{
			// now continue receving file headers, then their data
			if (recv(sockfd, buf, FILE_HEADER_BUF, MSG_WAITALL) < 0)
			{
				printf("myFTP> [client] Error in recv() for header\n");
				exit(EXIT_FAILURE);
			}

			if (buf[0] == 'L')
				to_break = 1;

			// conv binary to integer
			int left=buf[1];
			int right=buf[2];
			if(left<0){
				left+=256;
			}
			if(right<0){
				right+=256;
			}
			int sz=right+ left*(1<<8);
			// printf("%d %d %d received in client ^\n",sz,left,right);
			// now recv sz bytes of data
			// since this cab aribtarily long
			// so receive in chunks
			while (sz > 0)
			{
				int k;
				if ((k = recv(sockfd, buf, min(CHUNK_SIZE, sz), 0)) < 0)
				{
					printf("myFTP> [client] Error in recv() for file data\n");
					exit(EXIT_FAILURE);
				}
				if (k == 0)
					break;
				write(fd, buf, k);
				sz -= k;
			}
		}
	}
	else
	{
		*ok = 0;
		// printf("myFTP> [Response Code:%s] Error executing command\n", buf);
	}
}

void handle_get()
{
	if (!is_open)
	{
		printf("myFTP> 'open IP port' must be the first command\n");
		return;
	}
	// if (!is_login)
	// {
	// 	printf("myFTP> user not login, please login first.\n");
	// 	return;
	// }
	token[0][0]='\0';
	token[1][0]='\0';
	int index = 0;
	char *ptr = strtok(line, " ");
	ptr = strtok(NULL, " ");
	while (ptr != NULL && index < 2 && index < MAX_TOKENS)
	{
		strcpy(token[index++], ptr);
		ptr = strtok(NULL, " ");
	}

	// token[0] = remote_file
	// token[1] = local_file
	int ok=1;
	int *okptr = &ok;
	handle_get_each(token[0],token[1],okptr);
	if(ok){
		printf("myFTP> [Response Code:200] Command executed successfully\n");
	}else{
		printf("myFTP> [Response Code:500] Error executing command\n");
	}
}

void handle_put_each(char *local_file,char *remote_file,int *ok)
{

	int fd = open(local_file, O_RDONLY);
	if (fd < 0)
	{
		printf("myFTP> can't open [localfile : %s] for reading\n", token[1]);
		return;
	}

	/*
		generating the new command
	*/
	strcpy(line2, "put ");
	strcat(line2, local_file);
	strcat(line2, " ");
	strcat(line2, remote_file);
	n = strlen(line2);
	if (send(sockfd, line2, n + 1, 0) < 0)
	{
		printf("myFTP> error in send() from the client.\n");
		exit(EXIT_FAILURE);
	}

	// now receive
	// first receive 4 bytes to check response code
	if (recv(sockfd, buf, 4, MSG_WAITALL) < 0)
	{
		printf("myFTP> [client] Error in recv()\n");
		exit(EXIT_FAILURE);
	}

	if (!strcmp(buf, "200"))
	{
		while (1)
		{
			int k = read(fd, file_buffer, FILE_BUF);
			if (k < 0)
			{
				printf("error in reading file.\n");
				exit(EXIT_FAILURE);
			}
			if (k == 0)
				break;
			calc_binary(k);
			file_header[0] = 'M';
			// send header
			if (send(sockfd, file_header, FILE_HEADER_BUF, 0) < 0)
			{
				perror("[client] Error in send()\n");
				exit(EXIT_FAILURE);
			}
			// send data
			if (send(sockfd, file_buffer, k, 0) < 0)
			{
				perror("[client] Error in send()\n");
				exit(EXIT_FAILURE);
			}
		}
		calc_binary(0);
		file_header[0] = 'L';
		// send last block
		if (send(sockfd, file_header, FILE_HEADER_BUF, 0) < 0)
		{
			perror("[client] Error in send()\n");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		*ok=0;
	}
}

void handle_put()
{
	if (!is_open)
	{
		printf("myFTP> 'open IP port' must be the first command\n");
		return;
	}
	// if (!is_login)
	// {
	// 	printf("myFTP> user not login, please login first.\n");
	// 	return;
	// }
	token[0][0]='\0';
	token[1][0]='\0';
	int index = 0;
	char *ptr = strtok(line, " ");
	ptr = strtok(NULL, " ");
	while (ptr != NULL && index < 2 && index < MAX_TOKENS)
	{
		strcpy(token[index++], ptr);
		ptr = strtok(NULL, " ");
	}

	// token[0] = local_file
	// token[1] = remote_file
	int ok=1;
	int *okptr = &ok;
	handle_put_each(token[0],token[1],okptr);
	if(ok){
		printf("myFTP> [Response Code:200] Command executed successfully\n");
	}else{
		printf("myFTP> [Response Code:500] Error executing command\n");

	}
	
}

void handle_getline()
{
	printf("myFTP> ");
	fgets(line, LINE_BUFFER, stdin);
	n = strlen(line);

	// line may be ended by a '\n' , so putting that to '\0'
	if (line[n - 1] == '\n')
		line[n - 1] = '\0', n--;
}

void handle_invalid_command()
{
	printf("myFTP> Invalid Command\n");
}

void handle_mget(){
	if (!is_open)
	{
		printf("myFTP> 'open IP port' must be the first command\n");
		return;
	}
	// if (!is_login)
	// {
	// 	printf("myFTP> user not login, please login first.\n");
	// 	return;
	// }
	char *ptr = strtok(line, " ");
	ptr = strtok(NULL, " ");
	int ok=1;
	int *okptr = &ok;
	while (ptr != NULL && ok)
	{
		handle_get_each(ptr,ptr,okptr);
		ptr = strtok(NULL, " ");
	}

	if(ok){
		printf("myFTP> [Response Code:200] Command executed successfully\n");
	}else{
		printf("myFTP> [Response Code:500] Error executing command\n");

	}
}
void handle_mput(){
	if (!is_open)
	{
		printf("myFTP> 'open IP port' must be the first command\n");
		return;
	}
	// if (!is_login)
	// {
	// 	printf("myFTP> user not login, please login first.\n");
	// 	return;
	// }
	char *ptr = strtok(line, " ");
	ptr = strtok(NULL, " ");
	int ok=1;
	int *okptr = &ok;
	while (ptr != NULL && ok)
	{
		handle_put_each(ptr,ptr,okptr);
		ptr = strtok(NULL, " ");
	}

	if(ok){
		printf("myFTP> [Response Code:200] Command executed successfully\n");
	}else{
		printf("myFTP> [Response Code:500] Error executing command\n");

	}
}

int main()
{
	while (1)
	{
		handle_getline();
		if (!strncmp(line, "quit", 4))
			handle_quit();
		else if (!strncmp(line, "open", 4))
			handle_open();
		else if (!strncmp(line, "user", 4))
			handle_user();
		else if (!strncmp(line, "pass", 4))
			handle_pass();
		else if (!strncmp(line, "lcd", 3))
			handle_lcd();
		else if (!strncmp(line, "cd", 2))
			handle_cd();
		else if (!strncmp(line, "dir", 3))
			handle_dir();
		else if (!strncmp(line, "ldir", 4))
			handle_ldir();
		else if (!strncmp(line, "get", 3))
			handle_get();
		else if (!strncmp(line, "put", 3))
			handle_put();
		else if (!strncmp(line, "mget", 4))
			handle_mget();
		else if (!strncmp(line, "mput", 4))
			handle_mput();
		else
			handle_invalid_command();
	}

	return 0;
}
