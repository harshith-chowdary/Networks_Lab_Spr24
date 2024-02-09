#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_CLIENTS 5
#define BUFFER_SIZE 100

int cstrcmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        char c1 = tolower(*s1);
        char c2 = tolower(*s2);

        if (c1 != c2) {
            return c1 - c2;
        }

        s1++;
        s2++;
    }

    // If one string is longer than the other
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int search( char *name, char *basePath, char *ans){
    char path[1000];
    struct dirent *dp;
    DIR *dir = opendir(basePath);

    // Unable to open directory stream
    if (!dir){
        // printf("%s\n",dp->d_name);
        return 0;
    }

    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {
            // printf("%s\n", dp->d_name);

            // Construct new path from our base path
            
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dp->d_name);
            if((!strcmp(dp->d_name,name))){
                strcpy(ans, path);
                return 1;
            }
            int f=search(name, path, ans);
            if(f)
                return f;
        }
    }

    closedir(dir);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int my_port = atoi(argv[1]);
    int sockfd, newsockfd;
    int clilen;

    struct sockaddr_in cli_addr, serv_addr;

    // Create a socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Unable to create a socket !!\n");
        exit(0);
    }

    // Prepare server address structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(my_port);

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        perror("Unable to bind to local address !!\n");
        exit(0);
    }

    printf("SMTPMail Server is Running!!\n");

    listen(sockfd, 5);

    while(1){

        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if(newsockfd < 0){
            printf("Accept error !!\n");
            exit(0);
        }

        if(fork() == 0){
            char buf[BUFFER_SIZE];
            printf("\nConnected to Client : %s on Port %d\n\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            close(sockfd);

            // memset(buf, 0, BUFFER_SIZE);
            // recv(newsockfd, buf, BUFFER_SIZE, 0);

            // printf("C> %s", buf);

            memset(buf, 0, BUFFER_SIZE);
            sprintf(buf, "220 SMTPMail Service Ready\r\n");

            printf("S> %s", buf);

            send(newsockfd, buf, strlen(buf) + 1, 0);

            memset(buf, 0, BUFFER_SIZE);
            recv(newsockfd, buf, BUFFER_SIZE, 0);

            printf("C> %s", buf);

            char * tok = strtok(buf, " ");
            if(cstrcmp(tok, "HELO") != 0){
                printf("Error in HELO\n");

                memset(buf, 0, BUFFER_SIZE);
                sprintf(buf, "500 Syntax error, command unrecognized\r\n");

                printf("S> %s", buf);

                send(newsockfd, buf, strlen(buf) + 1, 0);
                close(newsockfd);
                exit(0);
            }

            tok = strtok(NULL, " ");
            tok = strtok(tok, "\r\n");
            char * host_domain = strdup(tok);

            memset(buf, 0, BUFFER_SIZE);
            sprintf(buf, "250 OK Hello %s\r\n", host_domain);

            printf("S> %s", buf);

            send(newsockfd, buf, strlen(buf) + 1, 0);

            memset(buf, 0, BUFFER_SIZE);
            recv(newsockfd, buf, BUFFER_SIZE, 0);

            printf("C> %s", buf);

            tok = strtok(buf, " ");
            if(cstrcmp(tok, "MAIL") != 0){
                printf("Error in MAIL\n");

                memset(buf, 0, BUFFER_SIZE);
                sprintf(buf, "500 Syntax error, command unrecognized\r\n");

                printf("S> %s", buf);

                send(newsockfd, buf, strlen(buf) + 1, 0);
                close(newsockfd);
                exit(0);
            }

            tok = strtok(NULL, " ");
            if(cstrcmp(tok, "FROM:") != 0){
                printf("Error in MAIL FROM\n");

                memset(buf, 0, BUFFER_SIZE);
                sprintf(buf, "500 Syntax error, command unrecognized\r\n");

                printf("S> %s", buf);

                send(newsockfd, buf, strlen(buf) + 1, 0);
                close(newsockfd);
                exit(0);
            }

            tok = strtok(NULL, " ");
            char * tmp = strdup(tok);
            tmp = strtok(tok, "\r\n");
            tok = strtok(tok, "<");
            tok = strtok(tok, "@");

            char * sender_user = strdup(tok);
            char receiver_file_path[BUFFER_SIZE];

            if(search(tok, ".", receiver_file_path)==0){
                printf("NO such USER <Sender> Found\n");

                memset(buf, 0, BUFFER_SIZE);
                sprintf(buf, "550 NO such USER <Sender> Found\r\n");

                printf("S> %s", buf);

                send(newsockfd, buf, strlen(buf) + 1, 0);
                close(newsockfd);
                exit(0);
            }

            memset(buf, 0, BUFFER_SIZE);
            sprintf(buf, "250 %s Sender OK\r\n", tmp);

            printf("S> %s", buf);

            send(newsockfd, buf, strlen(buf) + 1, 0);

            memset(buf, 0, BUFFER_SIZE);
            recv(newsockfd, buf, BUFFER_SIZE, 0);

            printf("C> %s", buf);

            tok = strtok(buf, " ");
            if(cstrcmp(tok, "RCPT") != 0){
                printf("Error in RCPT\n");

                memset(buf, 0, BUFFER_SIZE);
                sprintf(buf, "500 Syntax error, command unrecognized\r\n");

                printf("S> %s", buf);

                send(newsockfd, buf, strlen(buf) + 1, 0);
                close(newsockfd);
                exit(0);
            }

            tok = strtok(NULL, " ");
            if(cstrcmp(tok, "TO:") != 0){
                printf("Error in RCPT TO\n");

                memset(buf, 0, BUFFER_SIZE);
                sprintf(buf, "500 Syntax error, command unrecognized\r\n");

                printf("S> %s", buf);

                send(newsockfd, buf, strlen(buf) + 1, 0);
                close(newsockfd);
                exit(0);
            }

            tok = strtok(NULL, " ");
            tmp = strdup(tok);
            tmp = strtok(tok, "\r\n");

            tok = strtok(tok, "<");
            tok = strtok(tok, "@");

            char * receiver_user = strdup(tok);

            if(search(tok, ".", receiver_file_path)==0){
                printf("NO such USER <Receiver> Found\n");

                memset(buf, 0, BUFFER_SIZE);
                sprintf(buf, "550 NO such USER <Receiver> Found\r\n");

                printf("S> %s", buf);

                send(newsockfd, buf, strlen(buf) + 1, 0);
                close(newsockfd);
                exit(0);
            }

            memset(buf, 0, BUFFER_SIZE);
            sprintf(buf, "250 %s Receiver OK\r\n", tmp);

            printf("S> %s", buf);

            send(newsockfd, buf, strlen(buf) + 1, 0);

            memset(buf, 0, BUFFER_SIZE);
            recv(newsockfd, buf, BUFFER_SIZE, 0);

            printf("C> %s", buf);

            tok = strtok(buf, "\r\n");
            if(cstrcmp(buf, "DATA") != 0){
                printf("Error in DATA\n");

                memset(buf, 0, BUFFER_SIZE);
                sprintf(buf, "500 Syntax error, command unrecognized\r\n");

                printf("S> %s", buf);

                send(newsockfd, buf, strlen(buf) + 1, 0);
                close(newsockfd);
                exit(0);
            }

            memset(buf, 0, BUFFER_SIZE);
            sprintf(buf, "354 Start mail input; end with  '\\r\\n.\\r\\n' on a line by itself\r\n");

            printf("S> %s", buf);

            send(newsockfd, buf, strlen(buf) + 1, 0);

            strcat(receiver_file_path, "/mymailbox.txt");

            int file_desc = open(receiver_file_path, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);

            if (file_desc == -1){
                perror("Error opening file");

                memset(buf, 0, BUFFER_SIZE);
                sprintf(buf, "504 FILE ERROR\r\n");

                printf("S> %s", buf);

                send(newsockfd, buf, strlen(buf) + 1, 0);
                close(newsockfd);
                exit(0);
            }

            char eod[6] = "-----";
            int len;
            while(1){
                memset(buf, 0, BUFFER_SIZE);

                recv(newsockfd, buf, BUFFER_SIZE, 0);
                
                len = strlen(buf);

                // printf("C> %s", buf);

                // if(buf[0] == 0) break;

                ssize_t bytes_written = write(file_desc, buf, strlen(buf));

                if (bytes_written == -1)
                {
                    perror("Error writing to file");
                    close(file_desc);
                    memset(buf, 0, BUFFER_SIZE);
                    sprintf(buf, "504 FILE WRITE ERROR\r\n");

                    printf("S> %s", buf);

                    send(newsockfd, buf, strlen(buf) + 1, 0);
                    close(newsockfd);
                    exit(0);
                }

                if(len>=5) strncpy(eod, buf+len-5, 5);
                else{
                    strncpy(eod, eod+len, 5-len);
                    strncpy(eod+5-len, buf, len);
                }

                if(cstrcmp(eod, "\r\n.\r\n") == 0) break;
            }

            memset(buf, 0, BUFFER_SIZE);
            sprintf(buf, "250 OK Message accepted for delivery\r\n");

            printf("S> %s", buf);

            send(newsockfd, buf, strlen(buf) + 1, 0);

            close(file_desc);

            memset(buf, 0, BUFFER_SIZE);
            recv(newsockfd, buf, BUFFER_SIZE, 0);

            printf("C> %s", buf);

            tok = strtok(buf, "\r\n");
            if(cstrcmp(buf, "QUIT") != 0){
                printf("Error in QUIT\n");

                memset(buf, 0, BUFFER_SIZE);
                sprintf(buf, "500 Syntax error, command unrecognized\r\n");

                printf("S> %s", buf);

                send(newsockfd, buf, strlen(buf) + 1, 0);
                close(newsockfd);
                exit(0);
            }

            memset(buf, 0, BUFFER_SIZE);
            sprintf(buf, "221 '%s' Service closing transmission channel\r\n", host_domain);

            printf("S> %s", buf);

            send(newsockfd, buf, strlen(buf) + 1, 0);

            close(newsockfd);

            exit(0);
        }

        close(newsockfd);
    }

    close(sockfd);
    return 0;
}
