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
#define MAX_LINES 50

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

int check_user(const char *username, const char *password, int mode){
    FILE * fp = fopen("user.txt", "r");

    if(fp == NULL){
        printf("Unable to open file !!\n");
        exit(0);
    }

    char *tok;
    char line[BUFFER_SIZE];

    while(fgets(line, BUFFER_SIZE, fp) != NULL){
        tok = strtok(line, " ");
        if(strcmp(tok, username) == 0){
            if(mode == 0){
                fclose(fp);
                return 1;
            }
            
            tok = strtok(NULL, " |\n");
            if(strcmp(tok, password) == 0){
                fclose(fp);
                return 1;
            }

            return 0;
        }
    }

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

    printf("POP3 Server is Running!!\n");

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
            char *tok;

            printf("\nConnected to Client : %s on Port %d\n\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            close(sockfd);

            // memset(buf, 0, BUFFER_SIZE);
            // recv(newsockfd, buf, BUFFER_SIZE, 0);

            // printf("C> %s", buf);

            memset(buf, 0, BUFFER_SIZE);
            sprintf(buf, "+OK POP3 Server Ready ... !!\r\n");

            printf("S> %s", buf);

            send(newsockfd, buf, strlen(buf) + 1, 0);

            char username[BUFFER_SIZE], password[BUFFER_SIZE];

            char marked[1025];

            int mail_count = 0;

            int state = 0;

            while(1){
                memset(buf, 0, BUFFER_SIZE);
                recv(newsockfd, buf, BUFFER_SIZE, 0);

                if(strlen(buf) == 0) continue;
                printf("C> %s", buf);

                if(state==0){
                    tok = strtok(buf, " ");
                    if(cstrcmp(buf, "USER") == 0){
                        tok = strtok(NULL, " ");
                        tok = strtok(tok, "\r\n");

                        strcpy(username, tok);

                        if(check_user(username, password, 0) == 0)
                            sprintf(buf, "-ERR no such user here\r\n");
                        else
                            sprintf(buf, "+OK %s name okay, need password\r\n", username);

                        printf("S> %s", buf);

                        send(newsockfd, buf, strlen(buf) + 1, 0);
                    }
                    else if(cstrcmp(buf, "PASS") == 0){
                        tok = strtok(NULL, " ");
                        tok = strtok(tok, "\r\n");

                        strcpy(password, tok);

                        if(check_user(username, password, 1) == 0)
                            sprintf(buf, "-ERR password is invalid\r\n");
                        else {
                            sprintf(buf, "+OK %s and password Matched\r\n", username);
                            state = 1;
                        }

                        printf("S> %s", buf);

                        send(newsockfd, buf, strlen(buf) + 1, 0);
                    }
                    else{
                        memset(buf, 0, BUFFER_SIZE);
                        sprintf(buf, "-ERR Command not found\r\n");

                        printf("S> %s", buf);

                        send(newsockfd, buf, strlen(buf) + 1, 0);
                    }

                    continue;
                }

                if (state == 1) {
                    tok = strtok(buf, " |\r\n");

                    if (cstrcmp(buf, "STAT") == 0) {
                        // Open the mailbox file for reading
                        memset(buf, 0, BUFFER_SIZE);
                        sprintf(buf, "%s/mymailbox.txt", username);

                        // Open the mailbox file for reading
                        FILE *mailbox = fopen(buf, "r");
                        if (mailbox == NULL) {
                            perror("Error opening mailbox.txt");
                            exit(EXIT_FAILURE);
                        }

                        // Move to the beginning of the file
                        fseek(mailbox, 0, SEEK_SET);

                        int numMessages = 0;
                        int totalSize = 0;

                        // Count the number of messages and calculate their total size
                        // Count the number of messages
                        while (fgets(buf, BUFFER_SIZE, mailbox) != NULL) {
                            totalSize += strlen(buf);
                            // Check if the current line marks the end of an email message
                            if (strcmp(buf, ".\r\n") == 0) {
                                numMessages++;
                            }
                        }

                        // Close the mailbox file
                        fclose(mailbox);

                        memset(buf, 0, BUFFER_SIZE);
                        // Prepare the response

                        int c1 = htonl(numMessages);
                        int c2 = htonl(totalSize);
                        sprintf(buf, "+OK %d %d\r\n", c1, c2);

                        mail_count = numMessages;

                        // Send the response back to the client
                        send(newsockfd, buf, strlen(buf) + 1, 0);

                        printf("S> %s", buf);
                    }

                    else if (cstrcmp(tok, "RETR") == 0) {
                        // Extract the message number from the RETR command
                        tok = strtok(NULL, " |\r\n");
                        int messageNumber = atoi(tok);

                        messageNumber = ntohl(messageNumber);

                        memset(buf, 0, BUFFER_SIZE);
                        sprintf(buf, "%s/mymailbox.txt", username);

                        // Open the mailbox file for reading
                        FILE *mailbox = fopen(buf, "r");
                        if (mailbox == NULL) {
                            perror("Error opening mailbox.txt");
                            exit(EXIT_FAILURE);
                        }

                        // Move to the beginning of the file
                        fseek(mailbox, 0, SEEK_SET);

                        // printf("Reading message number %d\n", messageNumber);

                        // Search for the specified message number
                        int currentMessageNumber = 0;
                        int foundMessage = 0;
                        char msg[MAX_LINES][BUFFER_SIZE];

                        int line_no = 0;

                        while (fgets(msg[line_no], BUFFER_SIZE, mailbox) != NULL) {
                            // printf("Line %d: %s", line_no, msg[line_no]);

                            if (strcmp(msg[line_no], ".\r\n") == 0) {
                                // End of message, reset currentMessageNumber
                                currentMessageNumber++;
                                line_no++;

                                if (currentMessageNumber == messageNumber) {
                                    foundMessage = 1;
                                    break;
                                }
                                else{
                                    for(int i=0; i<line_no; i++)
                                        memset(msg[i], 0, BUFFER_SIZE);
                                    
                                    line_no = 0;
                                    continue;
                                }
                            }

                            line_no++;
                        }

                        // If the specified message number was not found, send an error response
                        if (!foundMessage) {
                            char errorResponse[] = "-ERR Message not found\r\n";
                            send(newsockfd, errorResponse, strlen(errorResponse), 0);
                        }

                        // If the specified message number was found, send the message to the client
                        else {
                            // Send the message to the client
                            for (int i = 0; i < line_no; i++) {
                                send(newsockfd, msg[i], BUFFER_SIZE, 0);

                                // printf("Sending: %s", msg[i]);
                            }
                        }

                        // Close the mailbox file
                        fclose(mailbox);
                    }

                    else if (cstrcmp(tok, "DELE") == 0) {
                        // Extract the message number from the DELE command
                        tok = strtok(NULL, " |\r\n");
                        int messageNumber = atoi(tok);
                        messageNumber = ntohl(messageNumber);

                        memset(buf, 0, BUFFER_SIZE);

                        if(marked[messageNumber] == 1){
                            sprintf(buf, "-ERR message %d already Marked to be DELETED\r\n", messageNumber);
                        }
                        else {
                            marked[messageNumber] = 1;
                            sprintf(buf, "+OK message %d Marked to be DELETED\r\n", messageNumber);
                        }

                        printf("S> %s", buf);

                        send(newsockfd, buf, strlen(buf) + 1, 0);
                    }

                    else if (strncmp(buf, "QUIT", 4) == 0) {
                        state = 2;
                    }

                    else {
                        memset(buf, 0, BUFFER_SIZE);
                        sprintf(buf, "-ERR Command not found\r\n");

                        printf("S> %s", buf);

                        send(newsockfd, buf, strlen(buf) + 1, 0);
                        
                    }
                }

                
                if(state==2) {
                    
                    memset(buf, 0, BUFFER_SIZE);
                    sprintf(buf, "%s/mymailbox.txt", username);

                    // Open the mailbox file for reading
                    FILE *mailbox = fopen(buf, "r");
                    if (mailbox == NULL) {
                        perror("Error opening mailbox.txt");
                        exit(EXIT_FAILURE);
                    }

                    memset(buf, 0, BUFFER_SIZE);
                    sprintf(buf, "%s/temp.txt", username);

                    FILE * temp = fopen(buf, "w");
                    if (temp == NULL) {
                        perror("Error opening temp.txt");
                        exit(EXIT_FAILURE);
                    }

                    // Move to the beginning of the file
                    fseek(mailbox, 0, SEEK_SET);

                    // printf("Reading message number %d\n", messageNumber);

                    // Search for the specified message number
                    int currentMessageNumber = 0;
                    int inMessage = 0;

                    while (fgets(buf, BUFFER_SIZE, mailbox) != NULL) {
                        // printf("Line %d: %s", line_no, msg[line_no]);

                        if (strncmp(buf, "From:", 5) == 0 || strncmp(buf, "DELETED", 7) == 0) {
                            // End of message, reset currentMessageNumber
                            currentMessageNumber++;
                        }

                        if(marked[currentMessageNumber]==1){
                            inMessage = 1;
                        }

                        if(marked[currentMessageNumber-1]==1 && inMessage){
                            // fprintf(temp, "DELETED\r\n.\r\n");
                            inMessage = 0;
                        }
                        
                        if(!inMessage){
                            fprintf(temp, "%s", buf);
                        }
                    }

                    for(int i=0; i<mail_count; i++)
                        if(marked[i] == 1) marked[i] = 0;

                    // Close the mailbox file
                    fclose(mailbox);
                    fclose(temp);

                    memset(buf, 0, BUFFER_SIZE);
                    sprintf(buf, "%s/mymailbox.txt", username);

                    if (remove(buf) == 0) {
                        printf("File deleted successfully.\n");
                    } else {
                        printf("Unable to delete the file.\n");
                    }

                    memset(buf, 0, BUFFER_SIZE);
                    sprintf(buf, "%s/temp.txt", username);

                    char newname[BUFFER_SIZE];
                    memset(newname, 0, BUFFER_SIZE);
                    sprintf(newname, "%s/mymailbox.txt", username);

                    if (rename(buf, newname) == 0) {
                        printf("File renamed successfully.\n");
                    } else {
                        printf("Unable to rename the file.\n");
                    }

                    // Prepare the response

                    memset(buf, 0, BUFFER_SIZE);
                    sprintf(buf, "+OK POP3 server signing off\r\n");

                    printf("S> %s", buf);

                    send(newsockfd, buf, strlen(buf) + 1, 0);
                    
                    close(newsockfd);

                    exit(0);
                }
            }

            close(newsockfd);
        }

        close(newsockfd);
    }

    close(sockfd);
    return 0;
}
