// A Simple Client Implementation for File Transfer
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <time.h>

#define MAXLINE 1024
#define FILENAME 100

int main() {
    int sockfd, err;
    struct sockaddr_in servaddr;
    int n;
    socklen_t len;
    char filename[FILENAME];
    char buffer[MAXLINE];

    // Creating socket file descriptor
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8181);
    err = inet_aton("127.0.0.1", &servaddr.sin_addr);
    // servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (err == 0) {
        printf("Error in ip-conversion\n");
        exit(EXIT_FAILURE);
    }

    printf("Enter the filename: ");
    fgets(filename, FILENAME-1, stdin);
    filename[strlen(filename)-1] = '\0';

    sendto(sockfd, (const char*)filename, strlen(filename), 0, (const struct sockaddr*)&servaddr, sizeof(servaddr));
    printf("\nFilename sent to server\n");

    len = sizeof(servaddr);
    n = recvfrom(sockfd, (char*)buffer, MAXLINE, 0, (struct sockaddr*)&servaddr, &len);
    buffer[n] = '\0';

    if (strstr(buffer, "NOTFOUND") != NULL) {
        printf("\nFile %s Not Found\n", buffer+9);
        close(sockfd);
        exit(EXIT_SUCCESS);
    }

    if (!(strstr(buffer, "HELLO"))) {
        printf("Server doesn't say HELLO\n");
        close(sockfd);
        exit(EXIT_SUCCESS);
    }

    printf("\nFile confirmed by Server\nCreating ");

    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    sprintf(filename, "localfile_%d:%d:%d_%d-%d-%d.txt", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900);
    printf("%s\n", filename);

    FILE* localFile = fopen(filename, "w+");

    // Uncomment this to print HELLO on the local file
    // fprintf(localFile, "%s", buffer);

    // if(buffer[strlen(buffer)-1] != 10) {
    //     fprintf(localFile, " ");
    // }

    int wordCount = 1;
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        sprintf(buffer, "WORD%d", wordCount);

        sendto(sockfd, (const char*)buffer, strlen(buffer), 0, (const struct sockaddr*)&servaddr, len);

        memset(buffer, 0, sizeof(buffer));

        n = recvfrom(sockfd, (char*)buffer, MAXLINE, 0, (struct sockaddr*)&servaddr, &len);

        if (strncmp(buffer, "END", 3) == 0) {
            // Uncomment this to print END on the local file
            // fprintf(localFile, "%s", buffer);
            printf("\nClosing local file.\n");

            sprintf(buffer, "DONE");
            sendto(sockfd, (const char*)buffer, strlen(buffer), 0, (const struct sockaddr*)&servaddr, len);
            break;
        }

        fprintf(localFile, "%s", buffer);
        
        wordCount++;
    }

    fclose(localFile);
    close(sockfd);
    return 0;
}
