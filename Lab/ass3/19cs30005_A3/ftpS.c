/*
	Networks Lab
	Group 47
	Assignment 6

	Aryan Agarwal, 19CS30005
	Vinit Raj, 19CS10065
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 200
#define COMMAND_BUFFER 200
#define RESULT_BUFFER 200
#define USER_BUFFER 200
#define FILE_BUF 200
#define FILE_HEADER_BUF 3 // 1 byte for 'M' or 'L' and 16 bits for length of frame
#define PORT 7000
#define CHUNK_SIZE 69

char command[COMMAND_BUFFER]; // We will use this buffer for receiving commands
char result[RESULT_BUFFER];	  // We will use this buffer for sending results
char buf[BUF_SIZE];			  // we will use this buffer for reading files
char user[USER_BUFFER];		  // we will use this buffer for reading user
char file_buffer[FILE_BUF];
char file_header[FILE_HEADER_BUF];

void handle_user(char *username)
{

	FILE *fp = fopen("user.txt", "r");
	if (fp == NULL)
	{
		// file not found, thus user not found
		strcpy(result, "500");
		return;
	}
	while (fgets(buf, BUF_SIZE, fp))
	{
		int n = strlen(buf);
		// line may be ended by a '\n'
		// so putting that to '\0'
		if (buf[n - 1] == '\n')
		{
			buf[n - 1] = '\0';
			n--;
		}
		char *ptr = strtok(buf, " ");
		if (!strcmp(username, ptr))
		{
			// if username matches result is 200
			strcpy(user, ptr);
			strcpy(result, "200");
			fclose(fp);
			return;
		}
	}
	// user not found
	strcpy(result, "500");
	fclose(fp);
	return;
}

void handle_pass(char *pswd)
{

	FILE *fp = fopen("user.txt", "r");
	if (fp == NULL)
	{
		// file not found, thus user not found
		strcpy(result, "500");
		return;
	}
	while (fgets(buf, BUF_SIZE, fp))
	{
		int n = strlen(buf);
		// line may be ended by a '\n'
		// so putting that to '\0'
		if (buf[n - 1] == '\n')
		{
			buf[n - 1] = '\0';
			n--;
		}
		char *ptr = strtok(buf, " ");
		if (!strcmp(user, ptr))
		{
			// if username matches
			// match pswd
			ptr = strtok(NULL, " ");
			if (!strcmp(pswd, ptr))
			{
				strcpy(result, "200"); // pswd match
			}
			else
			{
				strcpy(result, "500"); // pswd not match
			}
			fclose(fp);
			return;
		}
	}
	// user not found
	strcpy(result, "500");
	fclose(fp);
	return;
}

void handle_dir_change(char *ptr)
{
	if (chdir(ptr) < 0)
		strcpy(result, "500");
	else
		strcpy(result, "200");
}

void handle_dir(int newsockfd)
{
	struct dirent *directory_entry;
	DIR *dr = opendir(".");
	if (dr == NULL)
	{
		// Could not open current directory
		if (send(newsockfd, "500", 4, 0) < 0)
		{
			printf("[server] Error in send()\n");
			exit(EXIT_FAILURE);
		}
		return;
	}

	// first send null-terminated 200
	if (send(newsockfd, "200", 4, 0) < 0)
	{
		printf("[server] Error in send()\n");
		exit(EXIT_FAILURE);
	}
	while ((directory_entry = readdir(dr)) != NULL)
	{
		// now send null-terminated file names
		if (send(newsockfd, directory_entry->d_name, (int)strlen(directory_entry->d_name) + 1, 0) < 0)
		{
			printf("[server] Error in send()\n");
			exit(EXIT_FAILURE);
		}
	}

	// now send another null to denote end
	if (send(newsockfd, "", 1, 0) < 0)
	{
		printf("[server] Error in send()\n");
		exit(EXIT_FAILURE);
	}

	closedir(dr);
}

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

	int mask = (1 << 8) - 1;
	file_header[2] = mask & d;
	mask <<= 8;
	file_header[1] = mask & d;
}

void handle_get(char *remote_file, int newsockfd)
{

	int fd = open(remote_file, O_RDONLY);
	if (fd < 0)
	{
		// cant open remote file
		if (send(newsockfd, "500", 4, 0) < 0)
		{
			printf("[server] Error in send()\n");
			exit(EXIT_FAILURE);
		}
		return;
	}

	// send 200 first
	if (send(newsockfd, "200", 4, 0) < 0)
	{
		printf("[server] Error in send()\n");
		exit(EXIT_FAILURE);
	}

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
		// printf("%d sent\n", k);
		if (send(newsockfd, file_header, FILE_HEADER_BUF, 0) < 0)
		{
			perror("[server] Error in send()\n");
			exit(EXIT_FAILURE);
		}
		if (send(newsockfd, file_buffer, k, 0) < 0)
		{
			perror("[server] Error in send()\n");
			exit(EXIT_FAILURE);
		}
	}
	calc_binary(0);
	file_header[0] = 'L';

	if (send(newsockfd, file_header, FILE_HEADER_BUF, 0) < 0)
	{
		perror("[server] Error in send()\n");
		exit(EXIT_FAILURE);
	}
}

int min(int x, int y)
{
	return x < y ? x : y;
}

void handle_put(char *remote_file, int newsockfd)
{
	// printf("handle put called ins erver\n");

	int fd = open(remote_file, O_TRUNC | O_CREAT | O_WRONLY, 0644);

	/*
		O_TRUNC if file exists truncate it 0, as we want to overwrite
		O_CREAT create file if not exist in the specified path
		O_WRONLY only want write access
	*/
	if (fd < 0)
	{
		// cant open remote file
		if (send(newsockfd, "500", 4, 0) < 0)
		{
			printf("[server] Error in send()\n");
			exit(EXIT_FAILURE);
		}
		return;
	}

	// send 200 first
	if (send(newsockfd, "200", 4, 0) < 0)
	{
		printf("[server] Error in send()\n");
		exit(EXIT_FAILURE);
	}


	// now receive
	int to_break = 0;
	while (!to_break)
	{
		// now continue receving file headers, then their data
		if (recv(newsockfd, buf, FILE_HEADER_BUF, MSG_WAITALL) < 0)
		{
			printf("[server] Error in recv()\n");
			exit(EXIT_FAILURE);
		}

		if (buf[0] == 'L')
			to_break = 1;

		int left = buf[1];
		int right = buf[2];
		if (left < 0)
		{
			left += 256;
		}
		if (right < 0)
		{
			right += 256;
		}
		int sz = right + left * (1 << 8);
		// printf("%d %d %d received in server ^\n", sz, left, right);

		// now recv sz bytes of data
		// since this cab aribtarily long
		// so receive in chunks
		while (sz > 0)
		{
			int k;
			if ((k = recv(newsockfd, buf, min(CHUNK_SIZE, sz), 0)) < 0)
			{
				printf("[server] Error in recv()\n");
				exit(EXIT_FAILURE);
			}
			if (k == 0)
				break;
			write(fd, buf, k);
			sz -= k;
		}
	}
}

int main()
{
	int sockfd; // Socket descriptors
	struct sockaddr_in serv_addr;
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Cannot create socket\n");
		exit(0);
	}
	int opt = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		printf("error in setsockopt.\n");
		exit(EXIT_FAILURE);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);

	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("Unable to bind local address\n");
		exit(0);
	}
	if (listen(sockfd, 5) < 0)
	{
		perror("Unable to prepare to accept connections on socket FD.\n");
		exit(0);
	}

	printf("TCP Server successfully created on [PORT : %d] and [address : INADDR_ANY].\n", PORT);

	while (1)
	{
		int newsockfd;
		struct sockaddr_in cli_addr;
		int clilen = sizeof(cli_addr);
		if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) < 0)
		{
			perror("Accept error from client.\n");
			exit(EXIT_FAILURE);
		}

		printf("Successfully connected with client.\n");

		/*
			Having successfully accepted a client connection, the
			server now forks. The parent closes the new socket
			descriptor and loops back to accept the next connection.
		*/

		if (fork() == 0)
		{

			/*
				This child process will now communicate with the
				client through the send() and recv() system calls.
			*/

			/*
				Close the old socket since all
				communications will be through
				the new socket.
			*/
			close(sockfd);
			int is_user_match = 0;
			int is_pswd_match = 0;
			while (1)
			{
				/*
					copied from man recv

					These calls return the number of bytes received, or -1 if an error occurred.  In the event of an error, errno is set to indicate the error.
					When a stream socket peer has performed an orderly shutdown, the return value will be 0 (the traditional "end-of-file" return).
					Datagram sockets in various domains (e.g., the UNIX and Internet domains) permit zero-length datagrams.  When such a datagram is received, the return value is 0.
					The value 0 may also be returned if the requested number of bytes to receive from a stream socket was 0.
				*/

				/*
					recv() is a blocking call,
					so the server will keep on waiting for
					new bytes to be received at the newsockfd
					file descriptor, but as soon as the the
					connection gets closed from the client side
					recv will become non blocking and will read
					0 bytes of data each time, and go in an infinite
					loop, so check that also

				*/

				int k;
				if ((k = recv(newsockfd, command, COMMAND_BUFFER, 0)) < 0)
				{
					printf("[server] Error in recv()\n");
					exit(EXIT_FAILURE);
				}
				if (k == 0)
					break;

				if (!strncmp(command, "user", 4))
				{
					if (is_user_match)
					{
						// 'user' command can only be the first entered command
						strcpy(result, "600");
						if (send(newsockfd, result, strlen(result) + 1, 0) < 0)
						{
							printf("[server] Error in send()\n");
							exit(EXIT_FAILURE);
						}
						continue;
					}
					char *ptr = strtok(command, " "); // now ptr points to 0th token
					ptr = strtok(NULL, " ");		  // now ptr points to 1st token

					handle_user(ptr);

					if (!strcmp(result, "200"))
						is_user_match = 1;
					if (send(newsockfd, result, strlen(result) + 1, 0) < 0)
					{
						printf("[server] Error in send()\n");
						exit(EXIT_FAILURE);
					}
					continue;
				}

				if (!strncmp(command, "pass", 4))
				{
					if (!is_user_match || is_pswd_match)
					{
						// 'user' command can only be the first entered command
						// 'pass' command can only be the second entered command
						strcpy(result, "600");
						if (send(newsockfd, result, strlen(result) + 1, 0) < 0)
						{
							printf("[server] Error in send()\n");
							exit(EXIT_FAILURE);
						}
						continue;
					}
					char *ptr = strtok(command, " "); // now ptr points to 0th token
					ptr = strtok(NULL, " ");		  // now ptr points to 1st token
					handle_pass(ptr);

					if (!strcmp(result, "200"))
					{
						is_pswd_match = 1;
					}
					else
					{
						is_pswd_match = 0;
						is_user_match = 0;
					}
					if (send(newsockfd, result, strlen(result) + 1, 0) < 0)
					{
						printf("[server] Error in send()\n");
						exit(EXIT_FAILURE);
					}
					continue;
				}

				if (!strncmp(command, "cd", 2))
				{
					if (!is_user_match || !is_pswd_match)
					{
						// user currently not login
						strcpy(result, "500");
						if (send(newsockfd, result, strlen(result) + 1, 0) < 0)
						{
							printf("[server] Error in send()\n");
							exit(EXIT_FAILURE);
						}
						is_pswd_match = 0;
						is_user_match = 0;
						continue;
					}
					char *ptr = strtok(command, " ");
					ptr = strtok(NULL, " ");
					handle_dir_change(ptr);
					if (send(newsockfd, result, strlen(result) + 1, 0) < 0)
					{
						printf("[server] Error in send()\n");
						exit(EXIT_FAILURE);
					}
					continue;
				}

				if (!strncmp(command, "dir", 3))
				{
					if (!is_user_match || !is_pswd_match)
					{
						// user currently not login
						strcpy(result, "500");
						if (send(newsockfd, result, strlen(result) + 1, 0) < 0)
						{
							printf("[server] Error in send()\n");
							exit(EXIT_FAILURE);
						}
						is_pswd_match = 0;
						is_user_match = 0;
						continue;
					}
					handle_dir(newsockfd);
					continue;
				}
				if (!strncmp(command, "get", 3))
				{
					if (!is_user_match || !is_pswd_match)
					{
						// user currently not login
						strcpy(result, "500");
						if (send(newsockfd, result, strlen(result) + 1, 0) < 0)
						{
							printf("[server] Error in send()\n");
							exit(EXIT_FAILURE);
						}
						is_pswd_match = 0;
						is_user_match = 0;
						continue;
					}
					char *ptr = strtok(command, " ");
					ptr = strtok(NULL, " "); // ptr points to remote_file
					handle_get(ptr, newsockfd);
					continue;
				}
				if (!strncmp(command, "put", 3))
				{
					if (!is_user_match || !is_pswd_match)
					{
						// user currently not login
						strcpy(result, "500");
						if (send(newsockfd, result, strlen(result) + 1, 0) < 0)
						{
							printf("[server] Error in send()\n");
							exit(EXIT_FAILURE);
						}
						is_pswd_match = 0;
						is_user_match = 0;
						continue;
					}
					char *ptr = strtok(command, " ");
					ptr = strtok(NULL, " "); // ptr points to local_file
					ptr = strtok(NULL, " "); // ptr points to remote_file
					handle_put(ptr, newsockfd);
					continue;
				}
			}

			close(newsockfd);
			printf("Disconnected from client.\n");
			exit(EXIT_SUCCESS);
		}
		close(newsockfd);
	}

	close(sockfd);
	return 0;
}