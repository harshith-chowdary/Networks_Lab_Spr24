// A Simple UDP Server for File Transfer
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define MAXLINE 1024
#define FILENAME 100

char* searchFile(const char* filename, const char* directory) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;

    dir = opendir(directory);

    if (dir == NULL) {
        perror("opendir");
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        char path[MAXLINE];
        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        if (lstat(path, &statbuf) == -1) {
            perror("lstat");
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            // Ignore "." and ".." directories
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            // Recursively search in subdirectories
            char* result = searchFile(filename, path);
            if (result != NULL) {
                closedir(dir);
                return result;
            }
        } else if (S_ISREG(statbuf.st_mode)) {
            // Check if the current file matches the desired filename
            if (strcmp(entry->d_name, filename) == 0) {
                closedir(dir);
                return strdup(path);
            }
        }
    }

    closedir(dir);
    return NULL;
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    int n;
    socklen_t len;
    char buffer[MAXLINE];
    char countbuffer[MAXLINE];
    char filename[FILENAME];

    // Create socket file descriptor
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(8181);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("\nServer Running....\n");

    len = sizeof(cliaddr);

    while(1){
        n = recvfrom(sockfd, (char*)filename, sizeof(filename), 0, (struct sockaddr*)&cliaddr, &len);
        filename[n] = '\0';

        FILE* file = fopen(searchFile(filename, "."), "r");
        if (file == NULL) {
            printf("\nFile %s Not Found\n", filename);
            sprintf(buffer, "NOTFOUND %s", filename);
            sendto(sockfd, (const char*)buffer, strlen(buffer), 0, (const struct sockaddr*)&cliaddr, len);
            // close(sockfd);
            // exit(EXIT_SUCCESS);
            continue;
        }

        printf("\nFile %s Found\n", filename);

        int wordCount = 1;
        while (fscanf(file, "%1023s", buffer) == 1) {
            char nextChar = fgetc(file);
            int temp = strlen(buffer);

            printf("Sending %s\n", buffer);

            while(nextChar == 10 || nextChar == 32){
                buffer[temp++] = nextChar;
                nextChar = fgetc(file);
            }
            
            if(nextChar != EOF) {
                ungetc(nextChar, file);
            }

            sendto(sockfd, (const char*)buffer, strlen(buffer), 0, (const struct sockaddr*)&cliaddr, len);

            memset(buffer, 0, sizeof(buffer));
            n = recvfrom(sockfd, (char*)buffer, MAXLINE, 0, (struct sockaddr*)&cliaddr, &len);

            if(strcmp(buffer, "DONE") == 0) {
                printf("\nFile sent successfully\n");
                break;
            }

            memset(countbuffer, 0, sizeof(countbuffer));
            sprintf(countbuffer, "WORD%d", wordCount);

            if (strcmp(buffer, countbuffer) !=0) {
                printf("Invalid request received. Exiting.\n");
                strncpy(filename, buffer, FILENAME-1);
                break;
            }

            printf("Received   request %s\n", buffer);
            printf("Addressing request %s\n", buffer);

            wordCount++;

            memset(buffer, 0, sizeof(buffer));
        }

        fclose(file);
    }
    
    close(sockfd);
    return 0;
}
