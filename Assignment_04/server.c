#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#define MAX_CLIENTS 5
#define BUFFER_SIZE 100

int main(int argc, char * argv[]){
    int port = atoi(argv[1]);

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
    serv_addr.sin_port = htons(port);

    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        perror("Unable to bind to local address !!\n");
        exit(0);
    }

    printf("Server is Running!!\n");

    fd_set rfds;
    struct timeval tv;
    int retval;

    FD_ZERO(&rfds);
    FD_SET(0, &rfds);

    tv.tv_sec = 10;
    tv.tv_usec = 0;

    int nfds = 1;

    retval = select(nfds, &rfds, NULL, NULL, &tv);
    /* Don't rely on the value of tv now! */

    if (retval == -1)
        perror("select()");
    else if (retval)
        printf("Data is available now.\n");
        /* FD_ISSET(0, &rfds) will be true. */
    else
        printf("No data within %ld seconds.\n", tv.tv_sec);

    listen(sockfd, 5);

    while(1){

        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if(newsockfd>=nfds) nfds = newsockfd + 1;

        if(newsockfd < 0){
            printf("Accept error !!\n");
            exit(0);
        }

        if(fork() == 0){
            printf("\nConnected to Client : %s on Port %d\n\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            close(sockfd);
        }

        close(newsockfd);
    }

    close(sockfd);
    return 0;
}
