#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include <fcntl.h>

#define MAX 100

void encrypt(char * curr, int k){
    int tmp = *curr;

    if((tmp<65 || tmp>122) || (tmp>90 && tmp<97)) return;
    
    int flag = 0;

    if (tmp >= 'a'){
        tmp -= 'a';
        flag = 1;
    }
    else tmp -= 'A';

    tmp = (tmp + k) % 26;

    if(flag) tmp += 'a';
    else tmp += 'A';

    *curr = tmp;
}

int main(){
    int sockfd, newsockfd;
    int clilen;

    struct sockaddr_in cli_addr, serv_addr;

    int i;
    char buf[MAX];

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Unable to create a socket !!\n");
        exit(0);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(20000);

    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        perror("Unable to bind to local address !!\n");
        exit(0);
    }

    printf("Server started !!\n");

    listen(sockfd, 5);

    while(1){

        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if(newsockfd < 0){
            printf("Accept error !!\n");
            exit(0);
        }

        if(fork() == 0){

            printf("Connected to Client : %s on Port %d\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

            close(sockfd);

            memset(buf, 0, MAX);
            sprintf(buf, "Message from server");

            send(newsockfd, buf, strlen(buf) + 1, 0);

            memset(buf, 0, MAX);

            recv(newsockfd, buf, MAX, 0);

            printf("%s\n", buf);

            while(1){
                memset(buf, 0, MAX);

                recv(newsockfd, buf, MAX, 0);

                int k;

                k = atoi(buf);
                k = k%26;

                memset(buf, 0, MAX);

                sprintf(buf, "%s.%d.txt", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

                int file_desc = open(buf, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

                if (file_desc == -1)
                {
                    perror("Error opening file");
                    return 1;
                    exit(0);
                }

                // Writing to IP Temp file

                while(1){
                    memset(buf, 0, MAX);

                    int len;
                    len = recv(newsockfd, buf, MAX, 0);

                    if(buf[0] == 0) break;

                    int end = 0;
                    while(end<len && buf[end] != 0 && buf[end] != 64) end++;

                    ssize_t bytes_written = write(file_desc, buf, end);

                    if (bytes_written == -1)
                    {
                        perror("Error writing to file");
                        close(file_desc);
                        return 1;
                        exit(0);
                    }

                    if(end!=len) break;
                }

                printf("File received successfully from Client !!\n");
                close(file_desc);

                // Encrypting the file
                memset(buf, 0, MAX);

                sprintf(buf, "%s.%d.txt", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

                file_desc = open(buf, O_RDONLY);

                if (file_desc == -1)
                {
                    perror("Error opening file");
                    return 1;
                    exit(0);
                }

                memset(buf, 0, MAX);

                sprintf(buf, "%s.%d.txt.enc", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

                int wfile_desc = open(buf, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

                if (wfile_desc == -1)
                {
                    perror("Error opening file");
                    return 1;
                    exit(0);
                }

                lseek(file_desc, 0, SEEK_SET); // Move file cursor to the beginning

                memset(buf, 0, MAX);

                while (1){
                    ssize_t bytes_read = read(file_desc, buf, sizeof(buf));

                    if (bytes_read == -1){
                        perror("Error reading from file");
                        close(file_desc);
                        close(sockfd);
                        exit(0);
                    }

                    for(i=0; i<bytes_read; i++){
                        encrypt(buf+i, k);
                    }

                    ssize_t bytes_written = write(wfile_desc, buf, bytes_read);


                    if (bytes_written == -1)
                    {
                        perror("Error writing to file");
                        close(file_desc);
                        return 1;
                        exit(0);
                    }

                    if(bytes_read < MAX) break;

                    memset(buf, 0, MAX);
                }

                printf("Encryption Done Successfully !!\n");

                close(file_desc);
                close(wfile_desc);

                // Sending the encrypted file

                memset(buf, 0, MAX);

                sprintf(buf, "%s.%d.txt.enc", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

                file_desc = open(buf, O_RDONLY);

                if (file_desc == -1){
                    perror("Error opening file");
                    close(sockfd);
                    exit(0);
                }

                lseek(file_desc, 0, SEEK_SET); // Move file cursor to the beginning

                while (1){
                    ssize_t bytes_read = read(file_desc, buf, sizeof(buf));

                    if (bytes_read == -1){
                        perror("Error reading from file");
                        close(file_desc);
                        close(sockfd);
                        exit(0);
                    }

                    if(bytes_read < MAX){
                        buf[bytes_read] = 64;
                    }

                    send(newsockfd, buf, MAX, 0);

                    if(bytes_read < MAX) break;

                    memset(buf, 0, MAX); // ASCII value of '?'
                }

                printf("Encrypted File sent successfully from Server !!\n\n");

                close(file_desc);
                
                memset(buf, 0, MAX);

                recv(newsockfd, buf, MAX, 0);

                k = atoi(buf);

                if(k == 0) break;
            }
            exit(0);
        }

        close(newsockfd);
    }

    return 0;
}