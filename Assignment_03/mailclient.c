#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <time.h>

#define BUFFER_SIZE 100
#define MAX_LINES 50

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_IP> <smtp_port> <pop3_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    time_t rawtime;
    struct tm * timeinfo;

    char *server_ip = argv[1];
    int smtp_port = atoi(argv[2]);
    int pop3_port = atoi(argv[3]);

    printf("\nMail Client Started\n");

    char username[BUFFER_SIZE], password[BUFFER_SIZE];

    int login = 1; // Not required to do now so setting to 1

    while(1){

        if(!login){
            printf("\nLogin to your account\n");

            printf("Enter Username : ");
            fgets(username, BUFFER_SIZE, stdin);
            username[strlen(username)-1] = '\0';

            printf("Enter Password : ");
            fgets(password, BUFFER_SIZE, stdin);
            password[strlen(password)-1] = '\0';
        }
        
        printf("\nChoose any Option : [Default 3]\n1. Manage Mails\n2. Send Mail\n3. Quit\n");

        printf("Enter Choice : ");
        char ch[10];
        scanf("%s", ch);

        int choice = atoi(ch);

        if(choice!=1 && choice!=2) break;

        // Create socket
        int client_socket;
        if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(server_ip);

        if(choice==1){
            // Connect to pop3 port
            server_addr.sin_port = htons(pop3_port);

            // Manage Mails - POP3 Protocol
            continue;
        }

        // Connect to smpt port
        server_addr.sin_port = htons(smtp_port);

        // Connect to server
        if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("Connection failed");
            exit(EXIT_FAILURE);
        }

        char buf[BUFFER_SIZE];

        if(!login){
            // Send username to server
            memset(buf, 0, BUFFER_SIZE);
            sprintf(buf, "%s", username);
            send(client_socket, buf, strlen(buf), 0);

            // Send password to server
            memset(buf, 0, BUFFER_SIZE);
            sprintf(buf, "%s", password);
            send(client_socket, buf, strlen(buf), 0);

            // Receive response from server
            memset(buf, 0, BUFFER_SIZE);
            recv(client_socket, buf, BUFFER_SIZE, 0);

            if(buf[0]=='0') {
                printf("%s\n", buf+1);
                close(client_socket);
                continue;
            }

            login = 1;
        }

        // Send mail to server
        char msg[MAX_LINES][BUFFER_SIZE];

        printf("\nEnter mail to be sent (below) \n");

        getchar();

        int i = 0, len;
        while(i<MAX_LINES) {
            printf("<=> ");
            bzero(msg[i], 100);
            fgets(msg[i], 100, stdin);

            // printf("%s\n", msg[i]);

            len = strlen(msg[i-1]);

            if(i && msg[i-1][len-1]=='\n' && strcmp(msg[i], ".\n")==0) break;

            i++;
        }

        if(i>=MAX_LINES) {
            printf("Exceeded MAX Number of Lines : %d\n", MAX_LINES);

            close(client_socket);
            continue;
        }

        i++;
        printf("\nUser entered %d lines\n\n", i);

        char * tmp = strdup(msg[0]);
        char * tok = strtok(tmp, " ");

        if(strcmp(tok, "From:")!=0) {
            printf("tok = %s\n", tok);
            printf("Error in From !!\n");
            close(client_socket);
            continue;
        }

        tok = strtok(NULL, " ");
        if(tok==NULL) {
            printf("NO Sender id Found !!\n");
            close(client_socket);
            continue;
        }

        char * sender_domain = strdup(tok);
        tmp = strdup(msg[1]);

        tok = strtok(tmp, " ");
        if(strcmp(tok, "To:")!=0) {
            printf("Error in To\n");
            close(client_socket);
            continue;
        }

        tok = strtok(NULL, " |\n");
        if(tok==NULL) {
            printf("NO Receiver id Found !!\n");
            close(client_socket);
            continue;
        }

        char * receiver_domain = strdup(tok);
        tmp = strdup(msg[2]);

        tok = strtok(tmp, " |\n");
        if(strcmp(tok, "Subject:")!=0) {
            printf("Error in Subject\n");
            close(client_socket);
            continue;
        }

        // memset(buf, 0, BUFFER_SIZE);

        // sprintf(buf, "<client connects to SMTP Port>\r\n");

        // printf("C> %s", buf);

        // if (send(client_socket, buf, strlen(buf), 0) == -1) {
        //     perror("Send failed");
        //     close(client_socket);
        //     exit(EXIT_FAILURE);
        // }

        memset(buf, 0, BUFFER_SIZE);
        recv(client_socket, buf, BUFFER_SIZE, 0);

        printf("S> %s", buf);

        tok = strtok(buf, " ");
        if(strcmp(tok, "220")!=0) {
            printf("Error in Connection\n");
            close(client_socket);
            continue;
        }

        memset(buf, 0, BUFFER_SIZE);
        tok = strtok(sender_domain, "@");
        tok = strtok(NULL, "@|\n");
        sprintf(buf, "HELO %s\r\n", tok);

        char * sender_domain_only = strdup(tok);

        printf("C> %s", buf);

        if(send(client_socket, buf, strlen(buf), 0) == -1) {
            perror("Send failed");
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        memset(buf, 0, BUFFER_SIZE);
        recv(client_socket, buf, BUFFER_SIZE, 0);

        printf("S> %s", buf);

        tok = strtok(buf, " ");
        if(strcmp(tok, "250")!=0) {
            printf("Error in HELO\n");
            close(client_socket);
            continue;
        }

        memset(buf, 0, BUFFER_SIZE);
        sprintf(buf, "MAIL FROM: <%s@%s>\r\n", sender_domain, sender_domain_only);

        printf("C> %s", buf);

        if(send(client_socket, buf, strlen(buf), 0) == -1) {
            perror("Send failed");
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        memset(buf, 0, BUFFER_SIZE);
        recv(client_socket, buf, BUFFER_SIZE, 0);

        printf("S> %s", buf);

        tok = strtok(buf, " ");
        if(strcmp(tok, "250")!=0) {
            printf("Error in MAIL FROM\n");
            close(client_socket);
            continue;
        }

        memset(buf, 0, BUFFER_SIZE);
        sprintf(buf, "RCPT TO: <%s>\r\n", receiver_domain);
        // sprintf(buf, "RCPT TO: %s\r\n", receiver_domain);

        printf("C> %s", buf);

        if(send(client_socket, buf, strlen(buf), 0) == -1) {
            perror("Send failed");
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        memset(buf, 0, BUFFER_SIZE);
        recv(client_socket, buf, BUFFER_SIZE, 0);

        printf("S> %s", buf);

        tok = strtok(buf, " ");
        if(strcmp(tok, "250")!=0) {
            printf("Error in RCPT TO\n");
            close(client_socket);
            continue;
        }

        memset(buf, 0, BUFFER_SIZE);
        sprintf(buf, "DATA\r\n");

        printf("C> %s", buf);

        if(send(client_socket, buf, strlen(buf), 0) == -1) {
            perror("Send failed");
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        memset(buf, 0, BUFFER_SIZE);
        recv(client_socket, buf, BUFFER_SIZE, 0);

        printf("S> %s", buf);

        tok = strtok(buf, " ");
        if(strcmp(tok, "354")!=0) {
            printf("Error in DATA\n");
            close(client_socket);
            continue;
        }

        // Send mail to server
        int j;
        for(j=0; j<i; j++) {
            // printf("C> %s\n", msg[j]);
            msg[j][strlen(msg[j])-1] = '\r';
            msg[j][strlen(msg[j])] = '\n';
            if (send(client_socket, msg[j], strlen(msg[j]), 0) == -1) {
                perror("Send failed");
                close(client_socket);
                exit(EXIT_FAILURE);
            }

            if(j==2){
                time ( &rawtime );
                timeinfo = localtime ( &rawtime );

                memset(buf, 0, BUFFER_SIZE);
                sprintf(buf, "Received: %d-%d-%d @%d:%d\r\n", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900, timeinfo->tm_hour, timeinfo->tm_min);

                if (send(client_socket, buf, strlen(buf), 0) == -1) {
                    perror("Send failed");
                    close(client_socket);
                    exit(EXIT_FAILURE);
                }
            }
        }

        memset(buf, 0, BUFFER_SIZE);
        recv(client_socket, buf, BUFFER_SIZE, 0);

        printf("S> %s", buf);

        tok = strtok(buf, " ");
        if(strcmp(tok, "250")!=0) {
            printf("Error in DATA\n");
            close(client_socket);
            continue;
        }

        memset(buf, 0, BUFFER_SIZE);
        sprintf(buf, "QUIT\r\n");

        printf("C> %s", buf);

        if(send(client_socket, buf, strlen(buf), 0) == -1) {
            perror("Send failed");
            close(client_socket);
            exit(EXIT_FAILURE);
        }

        memset(buf, 0, BUFFER_SIZE);
        recv(client_socket, buf, BUFFER_SIZE, 0);

        printf("S> %s", buf);

        tok = strtok(buf, " ");

        if(strcmp(tok, "221")!=0) {
            printf("Error in QUIT\n");
            close(client_socket);
            continue;
        }

        // Close socket
        close(client_socket);
    }

    return 0;
}
