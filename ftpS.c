#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <fcntl.h>
#include <dirent.h>

#define MYPORT 15515
#define BACKLOG 10
#define BUF_SIZE 1000


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

void send_status (int sockfd, char* status)
{
    char *send_buf = (char *) calloc(BUF_SIZE, sizeof(char));
    // printf("Status: %s\n", status);
    strcat(send_buf, status);
    // printf("Send Buf: %s\n", send_buf);
    if (send(sockfd, send_buf, BUF_SIZE, 0) == -1)
        perror("send_status");

    sleep(0.5);
}


int main() {

    int sockfd, new_fd;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    int sin_size;
    int yes = 1;
    int numbytes;

    // Initializing the TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }
    
    // Reuse Address (Avoids "Address already in use" error)
    if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    // Defining servers address
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(MYPORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(my_addr.sin_zero), '\0', 8);

    // Binding the socket to the address
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }

    // Listening on the address for new connection
    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    sin_size = sizeof(struct sockaddr_in);


    // if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
    // {
    //     perror("accept");
    //     exit(121);
    // }

    // printf("Accepted Connection Successfully.\n");

    while (1)
    {
        fd_set fd;
        FD_ZERO(&fd);
        FD_SET(sockfd, &fd);        
        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        sleep(0.1);

        int retval;
        retval = select(sockfd+1, &fd, 0, 0, &tv);
        if (retval < 0)
        {
            perror("select");
            exit(1);
        }
        else if (retval == 0)
        {
            // Timeout
            continue;
        }

        if (FD_ISSET(sockfd, &fd))
        {
            int proc_no = fork();
            if (proc_no != 0)
            {
                // Parent Process
                continue;
            }

            if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
            {
                perror("accept");
                exit(1);
            }

            printf("Accepted Connection Successfully, Running on PID: %d\n", getpid());

            // States
            // 0: Not Opened
            // 1: Opened, Not Authenicated
            // 2: Opened, Username Entered, No Password
            // 3: Opened, Authecated
            int state = 1; // state = 0 is not possible here.

            char username[100], password[100];
            strcpy(username, "");
            strcpy(password, "");

            while (1) 
            {
                // Receive message into recv_buffer
                char *recv_buf = (char *) calloc(BUF_SIZE, sizeof(char));
                if ((numbytes=recv(new_fd, recv_buf, BUF_SIZE, 0)) == -1) 
                {
                    perror("recv");
                    exit(1);
                }
                if (numbytes == 0) 
                {
                    close(new_fd);
                    printf("QUIT: Closing Process: %d\n", getpid());
                    exit(0);
                }
                
                
                    
                // getting the command from the client in recv_buf
                char *cmd = strtok(recv_buf, " ");
                if (strcmp(cmd, "user") == 0)
                {
                    // TODO check if this is the first command
                    if (state != 1)
                    {
                        send_status(new_fd, "600");
                        continue;
                    }

                    // read the file to check if there is this user and store its corresponding password in "password"
                    int infd = open("user.txt", O_RDONLY);
                    if (infd == -1)
                    {
                        char cd[100];
                        printf("USER: Cannot find user.txt in %s\n", getcwd(cd, 100));
                    }

                    // char *username = recv_buf + 5;
                    strcpy(username, recv_buf + 5);

                    // printf("checking for the username %s\n", username);
                    char read_buf[100], curr_username[100], curr_password[100];
                    int username_index = 0, password_index = 0; // indices in the curr_username and curr_password
                    int is_reading_username = 0; // 0 while reading username, 1 while reading password
                    while (read(infd, read_buf, 100) > 0)
                    {
                        // check for newlines, they mark the start of a new username-password pair
                        for (int i = 0; i < 100; i++)
                        {
                            if (read_buf[i] == '\n')
                            {
                                // username and password over, new pair will begin
                                curr_username[username_index] = '\0';
                                curr_password[password_index] = '\0';
                                // printf("the username password pair is %s, %s\n", curr_username, curr_password);
                                if (strcmp(curr_username, username) == 0)
                                {
                                    // save the corresponding password for later use
                                    strcpy(password, curr_password);
                                    // printf("the corresponding password is found to be %s from the user.txt file\n", password);
                                }
                                is_reading_username = 0;
                                username_index = 0;
                                password_index = 0;
                            }
                            else if (read_buf[i] == ' ')
                            {
                                is_reading_username = 1; // password occurs after one or more spaces
                            }
                            else if (is_reading_username == 0)
                            {
                                curr_username[username_index++] = read_buf[i];
                            }
                            else
                            {
                                curr_password[password_index++] = read_buf[i];
                            }
                        }
                    }
                    if (strcmp(password, "") == 0)
                    {
                        // username not found in the file user.txt
                        // send 500
                        // TODO check if strlen(retval) or some big number
                        send_status(new_fd, "500");
                        state = 1;
                    }
                    else
                    {
                        // username is found and the corresponding password is written in "password"
                        // send 200
                        // TODO check if strlen(retval) or some big number
                        send_status(new_fd, "200");
                        state = 2;
                    }
                }
                if (strcmp(cmd, "pass") == 0)
                {
                    // TODO check if this is the second command
                    if (state != 2)
                    {
                        send_status(new_fd, "600");
                        continue;
                    }

                    // printf("password is supposed to be %s\n", password);
                    
                    // check password
                    if (strcmp(password, recv_buf + 5) == 0)
                    {
                        // password correct, send 200
                        state = 3;
                        // TODO check if strlen(retval) or some big number
                        send_status(new_fd, "200");
                    }
                    else
                    {
                        // password incorrect, send 500
                        state = 1;
                        strcpy(password, "");
                        
                        // TODO check if strlen(retval) or some big number
                        send_status(new_fd, "500");
                    }
                }
                if (strcmp(cmd, "cd") == 0)
                {
                    if (state != 3)
                    {
                        send_status(new_fd, "600");
                        continue;
                    }
                    // printf("received %s from the client\n", recv_buf);
                    char directory[200];
                    strcpy(directory, recv_buf + 3);
                    chdir(directory);
                    printf("DIR: Current directory is %s\n", getcwd(directory, 200));

                    
                    // TODO check if strlen(retval) or some big number
                    send_status(new_fd, "200");
                }
            

                if (strcmp(cmd, "get") == 0)
                {
                    // printf("received get %s %s from the client\n", strtok(NULL, " "), strtok(NULL, " "));
                    char *rf = strtok(NULL, " ");
                    char *lf = strtok(NULL, " ");

                    if (state != 3)
                    {
                        send_status(new_fd, "600");
                        continue;
                    }

                    int readfd;
                    if ((readfd = open(rf, O_RDONLY | O_EXCL)) == -1)
                    {
                        printf("GET: File \"%s\" not found.\n", rf);
                        send_status(new_fd, "500");

                        continue;

                    }

                    send_status(new_fd, "200");

                    char *read_buf = (char *) calloc(BUF_SIZE-3, sizeof(char));
                    // char *actual_read_buf = (char *) calloc(BUF_SIZE-3, sizeof(char));
                    int num_bytes;
                    int total = 0;
                    while ((num_bytes = read(readfd, read_buf, BUF_SIZE-3)))
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

                        if (send(new_fd, send_buf, BUF_SIZE, 0) == -1)
                            perror("send");

                    }
                    
                }

                if (strcmp(cmd, "put") == 0)
                {
                    char *lf = strtok(NULL, " ");
                    char *rf = strtok(NULL, " ");

                    if (state != 3)
                    {
                        send_status(new_fd, "600");
                        continue;
                    }

                    int write_fd;
                    if((write_fd = open(rf, O_WRONLY | O_CREAT, 0777)) == -1)
                    {
                        printf("PUT: File \"%s\" can not be opened.\n", rf);
                        send_status(new_fd, "500");
                        continue;
                    }

                    send_status(new_fd, "200");

                    char *recv_buf;
                    do
                    {
                        int numbytes;
                        recv_buf = (char *) calloc(BUF_SIZE, sizeof(char));
                        if ((numbytes=recv(new_fd, recv_buf, BUF_SIZE, 0)) == -1) 
                        {
                            perror("recv");
                            exit(1);
                        }

                        int msg_len = decode_int(recv_buf+1);
                        // printf("msg_len: %d\n", msg_len);
                        // printf("Ok\n");
                        if (write(write_fd, recv_buf+3, msg_len) == -1)
                        {
                            perror("write");
                        }

                    }
                    while (recv_buf[0] == 'M');
                }

                if (strcmp(recv_buf, "dir") == 0)
                {
                    // if (state != 3)
                    // {
                    //     send_status(new_fd, "600");
                    //     continue;
                    // }

                    // printf("received %s from the client\n", recv_buf);
                    struct dirent * file;
                    DIR * dir = opendir(".");
                    while ((file = readdir(dir)) != NULL)
                    {
                        // printf("-->%s\n", file->d_name);
                        if (send(new_fd, file->d_name, strlen(file->d_name) + 1, 0) == -1)
                        {
                            perror("send");
                        }
                    }
                    char filename[] = "";
                    if (send(new_fd, filename, strlen(filename) + 1, 0) == -1)
                    {
                        perror("send");
                    }
                }
            }
        }
    }



}