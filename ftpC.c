#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <sys/types.h>
#include <libgen.h> 
#include <sys/socket.h> 
#include <sys/select.h>
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define BUF_SIZE 1000

int checkStatus(int state)
{
	if (state == 0)
	{
		printf("Error: Connection is not opened.\n");
		return 0;
	}
	else if (state < 3)
	{
		printf("Error: Connection is not authenticated.\n");
		return 0;
	}

	return 1;
}

void encode_int (char *str, int num)
{
    str[1] = ((num) & 0xFF);
    str[0] = (num >> 8) & 0xFF;
}

uint decode_int (char *str)
{
    uint temp = (unsigned char) str[0];
    temp <<= 8;
    temp += (unsigned char) str[1];

    return temp;
}

uint get_status (int sockfd)
{
	int numbytes;
	char *recv_buf = (char *) calloc(BUF_SIZE, sizeof(char));
	if ((numbytes=recv(sockfd, recv_buf, BUF_SIZE, 0)) == -1) 
	{
		perror("recv");
		exit(1);
	}

	// printf("%d\n", atoi(recv_buf));
	// printf("%d\n", numbytes);
	// printf("%s\n", recv_buf);

	return atoi(recv_buf);
}

int main()
{
	int sockfd;

	// States
	// 0: Not Opened
	// 1: Opened, Not Authenicated
	// 2: Opened, Username Entered, No Password
	// 3: Opened, Authecated
	int state = 0;

	while (1)
	{
		printf("myFTP> ");
		char command[500];
		scanf("%s", command);

		if (strcmp(command, "open") == 0)
		{
			// TODO: Dont HardCode

			char ip[20];
			int port;
			scanf("%s %d", ip, &port);

			if (state != 0)
			{
				printf("OPEN: Connection already opened.\n");
			}

			// open a connection

			
			struct sockaddr_in servaddr;
			sockfd = socket(AF_INET, SOCK_STREAM, 0);
			if (sockfd < 0)
			{
				perror("Socket Creation Failed");
				exit(0);
			}

			memset(&servaddr, 0, sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_port = htons(port);
			inet_aton(ip, &servaddr.sin_addr);
			if ((connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))) < 0)
			{
				perror("Unable to Connect to Server");
				exit(0);
			}
			printf("OPEN: Connection Successful\n");
			state = 1;
				
		}
		if (strcmp(command, "user") == 0)
		{
			char username[200];
			scanf("%s", username);

			// send to server
			strcat(command, " ");
			strcat(command, username);
			// printf("sending server %s\n", command);
            // TODO strlen(command) or 200/500?
			if (send(sockfd, command, strlen(command), 0) == -1)
			{
				perror("send");
			}

            // char recv_buf[BUF_SIZE];
            // if (recv(sockfd, recv_buf, BUF_SIZE, 0) == -1) 
            // {
            //     perror("recv");
            //     exit(1);
            // }
            // printf("received %s from the server\n", recv_buf);
			int return_status = get_status(sockfd);
			// printf("%d\n", return_status);
            if (return_status == 200)
            {
                printf("USER: Command Executed Successfully\n");
                state = 2;
            }
            else if (return_status == 500)
            {
                printf("USER: Error Executing Command\n");
                state = 1;
            }
            else if (return_status == 600)
            {
                printf("USER: User name already entered\n");
            }

		}
		if (strcmp(command, "pass") == 0)
		{
			char password[200];
			scanf("%s", password);

			// send to server
            strcat(command, " ");
			strcat(command, password);
			// printf("sending server %s\n", command);
            // TODO strlen(command) or 200/500?
			if (send(sockfd, command, strlen(command), 0) == -1)
			{
				perror("send");
			}

        	int return_status = get_status(sockfd);
            if (return_status == 200)
            {
                printf("PASS: Command Executed Successfully\n");
                state = 3;
            }
            else if (return_status == 500)
            {
                printf("PASS: Error Executing Command\n");
                state = 1;
            }
            else if (return_status == 600)
            {
                printf("PASS: Username not entered or Already authenticated\n");
            }
		}
		if (strcmp(command, "cd") == 0)
		{
			char path[200];
			scanf("%s", path);

			if (!checkStatus(state))
				continue;

			// send to server
            strcat(command, " ");
			strcat(command, path);
			// printf("sending server %s\n", command);
            // TODO strlen(command) or 200/500?
			if (send(sockfd, command, strlen(command), 0) == -1)
			{
				perror("send");
			}

            int return_status = get_status(sockfd);
            if (return_status == 200)
            {
                printf("CD: Command Executed Successfully\n");
                state = 3;
            }
            else if (return_status == 500)
            {
                printf("CD: Error Executing Command\n");
            }
            else if (return_status == 600)
            {
                printf("CD: User not Authenticated\n");
            }
		}
		if (strcmp(command, "lcd") == 0)
		{
			char path[200];
			scanf("%s", path);

			if (!checkStatus(state))
				continue;

			chdir(path);
            printf("LCD: Current directory is %s\n", getcwd(path, 200));
		}
		if (strcmp(command, "dir") == 0)
		{
			if (!checkStatus(state))
				continue;
			
			// send to server
            // printf("sending server %s\n", command);
            // TODO strlen(command) or 200/500?
			if (send(sockfd, command, strlen(command), 0) == -1)
			{
				perror("send");
			}

			// print the result
            char recv_buf[BUF_SIZE];
            char total_buf[10 * BUF_SIZE];
            int total_buf_ptr = 0;
            int numbytes;
            printf("DIR: The list of files:\n");
            while ((numbytes = recv(sockfd, recv_buf, BUF_SIZE, 0)) > 0) 
            {
                // if (strcmp(recv_buf, "") == 0)
                // {
                //     break;
                // }
                // printf("%s\n", recv_buf);
                for (int i = 0; i < numbytes; i++)
                {
                    total_buf[total_buf_ptr++] = recv_buf[i];
                }
                if (total_buf[0] == '\0')
                {
                    break;
                    // empty dir
                }
                if (total_buf_ptr >= 2)
                {
                    if (total_buf[total_buf_ptr - 1] == '\0' && total_buf[total_buf_ptr - 2] == '\0')
                    {
                        break;
                    }
                }
            }
            if (numbytes == -1)
            {
                perror("recv");
                exit(1);
            }
			printf("\t");
            for (int i = 0; i < total_buf_ptr - 1; i++)
            {
                if (total_buf[i] == '\0')
                {
                    printf("\n\t");
                }
                else
                {
                    printf("%c", total_buf[i]);
                }
            }
            
		}
		if (strcmp(command, "get") == 0)
		{
			char lf[100], rf[100];
			scanf("%s %s", rf, lf);

			if (!checkStatus(state))
				continue;

			// try opening, if opened send to server and read contents
			// File in current directory or absolute path, no relative
			// Check if can be opened, send command
			// if remote file present send 200, data
			// else send 500

			int write_fd;
			if((write_fd = open(lf, O_WRONLY | O_CREAT, 0777)) == -1)
			{
				perror("open");
				continue;
			}

			char *send_buf = (char *) calloc(BUF_SIZE, sizeof(char));
			strcat(send_buf, "get ");
			strcat(send_buf, rf);
			strcat(send_buf, " ");
			strcat(send_buf, lf);
			if (send(sockfd, send_buf, strlen(send_buf), 0) == -1)
				perror("send");

			int return_status = get_status(sockfd);
			if (return_status == 500)
			{
				printf("GET: Remote file \"%s\" can not be opened.\n", rf);
				continue;
			}
			else if (return_status == 200)
			{
				char *recv_buf;
				do
				{
					int numbytes;
					recv_buf = (char *) calloc(BUF_SIZE, sizeof(char));
					if ((numbytes=recv(sockfd, recv_buf, BUF_SIZE, 0)) == -1) 
					{
						perror("recv");
						exit(1);
					}

					int msg_len = decode_int(recv_buf+1);
					// printf("msg_len: %d, num_bytes: %d\n", msg_len, numbytes);

					if (write(write_fd, recv_buf+3, msg_len) == -1)
					{
						perror("write");
					}

				}
				while (recv_buf[0] == 'M');
			}
			else if (return_status == 600)
            {
                printf("GET: User not Authenticated\n");
				continue;
            }

			printf("GET: Successfully download \"%s\" as \"%s\"\n", rf, lf);

		}
		if (strcmp(command, "put") == 0)
		{
			char lf[100], rf[100];
			scanf("%s %s", lf, rf);

			if (!checkStatus(state))
				continue;

			// try opening, if opened send to server and write contents
			int read_fd;
			if((read_fd = open(lf, O_RDONLY | O_EXCL, 0777)) == -1)
			{
				perror("open");
				continue;
			}

			char *send_buf = (char *) calloc(BUF_SIZE, sizeof(char));
			strcat(send_buf, "put ");
			strcat(send_buf, lf);
			strcat(send_buf, " ");
			strcat(send_buf, rf);
			if (send(sockfd, send_buf, strlen(send_buf), 0) == -1)
				perror("send");

			int return_status = get_status(sockfd);
			if (return_status == 500)
			{
				printf("PUT: Remote file \"%s\" can not be opened.\n", rf);
				continue;
			}
			else if (return_status == 200)
			{
				char *read_buf = (char *) calloc(BUF_SIZE-3, sizeof(char));
				int num_bytes;
				int total = 0;
				while ((num_bytes = read(read_fd, read_buf, BUF_SIZE-3)))
				{
					// memcpy(actual_read_buf, read_buf)
					char* send_buf = (char *) calloc(BUF_SIZE, sizeof(char));

					if (num_bytes < BUF_SIZE - 3)
					{
						strcat(send_buf, "L");
					}
					else if (num_bytes == BUF_SIZE - 3)
					{
						strcat(send_buf, "M");
					}

					
					encode_int(send_buf+1, num_bytes);

					// printf("Decoded: %d\n", decode_int(send_buf+1));
					

					memcpy(send_buf+3, read_buf, num_bytes);

					if (send(sockfd, send_buf, BUF_SIZE, 0) == -1)
						perror("send");

					sleep(0.5);
				}
			}
			else if (return_status == 600)
            {
                printf("PUT: User not Authenticated\n");
				continue;
            }

			printf("PUT: Successfully uploaded \"%s\" as \"%s\"\n", lf, rf);

		}
		if (strcmp(command, "mget") == 0)
		{
			char files[200];
			scanf("%[^\n]", files);

			if (!checkStatus(state))
				continue;

            int failed = 0;

			// do something with strtok
            // call get on all the files
            char delimiter[] = " ";
            char *current_file = strtok(files, delimiter);
            // printf("The files are : ");
            while (current_file != NULL)
            {
                // printf("%s ",  current_file);

                char send_buf[BUF_SIZE];
                strcpy(send_buf, "get ");
				char * rf = current_file;
                strcat(send_buf, rf);
                strcat(send_buf, " ");
                char* lf = basename(current_file);
                strcat(send_buf, lf);
                if (send(sockfd, send_buf, BUF_SIZE, 0) == -1)
                {
                    perror("send");
                }

				int write_fd;
				if((write_fd = open(lf, O_WRONLY | O_CREAT, 0777)) == -1)
				{
					perror("open");
					continue;
				}

                int return_status = get_status(sockfd);
                if (return_status == 500)
                {
                    failed = 1;
                    break;
                }
				else if (return_status == 200)
				{
					char *recv_buf;
					do
					{
						int numbytes;
						recv_buf = (char *) calloc(BUF_SIZE, sizeof(char));
						if ((numbytes=recv(sockfd, recv_buf, BUF_SIZE, 0)) == -1) 
						{
							perror("recv");
							exit(1);
						}

						int msg_len = decode_int(recv_buf+1);
						// printf("msg_len: %d\n", msg_len);

						if (write(write_fd, recv_buf+3, msg_len) == -1)
						{
							perror("write");
						}

					}
					while (recv_buf[0] == 'M');
				}
				else if (return_status == 600)
				{
					printf("MGET: User not Authenticated\n");
					continue;
				}

				printf("MGET: Successfully downloaded \"%s\" as \"%s\"\n", rf, lf);

                current_file = strtok(NULL, delimiter);
            }

            if (failed)
            {
                printf("MGET: Error Executing Command\n");
            }
            else
            {
                printf("MGET: Command Executed Successfully\n");
            }

		}
		if (strcmp(command, "mput") == 0)
		{
			char files[200];
			scanf("%[^\n]", files);

			if (!checkStatus(state))
				continue;

            int failed = 0;

			// do something with strtok
            // call get on all the files
            char delimiter[] = " ";
            char *current_file = strtok(files, delimiter);
            // printf("The files are : ");
            while (current_file != NULL)
            {
                char send_buf[BUF_SIZE];
                strcpy(send_buf, "put ");
				char* lf = current_file;
                strcat(send_buf, lf);
                strcat(send_buf, " ");
                char* rf = basename(current_file);
                strcat(send_buf, rf);
                if (send(sockfd, send_buf, BUF_SIZE, 0) == -1)
                {
                    perror("send");
                }

				int read_fd;
				if((read_fd = open(lf, O_RDONLY | O_EXCL, 0777)) == -1)
				{
					perror("open");
					continue;
				}

                int return_status = get_status(sockfd);
                if (return_status == 500)
                {
                    failed = 1;
                    break;
                }
				else if (return_status == 200)
				{
					char *read_buf = (char *) calloc(BUF_SIZE-3, sizeof(char));
					int num_bytes;
					int total = 0;
					while ((num_bytes = read(read_fd, read_buf, BUF_SIZE-3)))
					{
						// memcpy(actual_read_buf, read_buf)
						char* send_buf = (char *) calloc(BUF_SIZE, sizeof(char));

						if (num_bytes < BUF_SIZE - 3)
						{
							strcat(send_buf, "L");
						}
						else if (num_bytes == BUF_SIZE - 3)
						{
							strcat(send_buf, "M");
						}

						
						encode_int(send_buf+1, num_bytes);

						// printf("Decoded: %d\n", decode_int(send_buf+1));
						

						memcpy(send_buf+3, read_buf, num_bytes);

						if (send(sockfd, send_buf, BUF_SIZE, 0) == -1)
							perror("send");

						sleep(0.5);
					}
				}
				else if (return_status == 600)
				{
					printf("MPUT: User not Authenticated\n");
					continue;
				}

                // printf("%s ",  current_file);
				printf("MPUT: Successfully uploaded \"%s\" as \"%s\"\n", lf, rf);
                current_file = strtok(NULL, delimiter);
            }
			

            if (failed)
            {
                printf("MPUT: Error Executing Command\n");
            }
            else
            {
                printf("MPUT: Command Executed Successfully\n");
            }

		}
		if (strcmp(command, "quit") == 0)
		{
			// close the connection
			if (state == 0)
			{
				printf("OUT: No open connection to close\n");
				continue;
			}
			state = 0;
			close(sockfd);
		}


	}
}