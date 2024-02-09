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

    printf("\nMail Client Started, press ENTER to start ..");

    char username[BUFFER_SIZE], password[BUFFER_SIZE];

    int login = 0; // Not required to do now so setting to 1

    struct sockaddr_in smtp_server_addr;
    memset(&smtp_server_addr, 0, sizeof(smtp_server_addr));
    smtp_server_addr.sin_family = AF_INET;
    smtp_server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // Connect to smpt port
    smtp_server_addr.sin_port = htons(smtp_port);

    struct sockaddr_in pop3_server_addr;
    memset(&pop3_server_addr, 0, sizeof(pop3_server_addr));
    pop3_server_addr.sin_family = AF_INET;
    pop3_server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // Connect to pop3 port
    pop3_server_addr.sin_port = htons(pop3_port);

    char buf[BUFFER_SIZE];
    char * tok;

    int choice;
    int done = 0;

    int pop3_socket, smtp_socket;

    while(1){

        if(!login){
            fflush(stdin);
            fgets(buf, BUFFER_SIZE, stdin);

            if(done) {
                printf("Enter 0 to quit (1 or any other to continue): ");
                char ch[10];
                scanf("%s", ch);

                choice = atoi(ch);

                if(choice==0) break;

                getchar();
            }

            printf("\nLogin to your account\n");

            printf("Enter Username : ");
            fgets(username, BUFFER_SIZE, stdin);
            username[strlen(username)-1] = '\0';

            printf("Enter Password : ");
            fgets(password, BUFFER_SIZE, stdin);
            password[strlen(password)-1] = '\0';

            choice = -1;

            printf("\n");
    
            done = 1;

            // Create POP3 socket
            if ((pop3_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                perror("POP3 Socket creation failed");
                exit(EXIT_FAILURE);
            }

            // Connect to server
            if (connect(pop3_socket, (struct sockaddr *)&pop3_server_addr, sizeof(pop3_server_addr)) == -1) {
                perror("Connection failed");
                exit(EXIT_FAILURE);
            }

            memset(buf, 0, BUFFER_SIZE);
            recv(pop3_socket, buf, BUFFER_SIZE, 0);

            printf("S> %s", buf);

            char * tok = strtok(buf, " ");

            if(strcmp(tok, "+OK")!=0) {
                printf("Error in Connection to POP3 Server\n");
                close(pop3_socket);
                continue;
            }

            memset(buf, 0, BUFFER_SIZE);
            sprintf(buf, "USER %s\r\n", username);

            printf("C> %s", buf);

            if (send(pop3_socket, buf, strlen(buf), 0) == -1) {
                perror("Send failed");
                close(pop3_socket);
                exit(EXIT_FAILURE);
            }

            memset(buf, 0, BUFFER_SIZE);
            recv(pop3_socket, buf, BUFFER_SIZE, 0);

            printf("S> %s", buf);

            tok = strtok(buf, " |\r\n");
            if(strcmp(tok, "+OK")!=0) {
                printf("Error in USER, No such user found !!\n");

                printf("\nClick Enter to Continue ..\n");
                close(pop3_socket);
                continue;
            }

            memset(buf, 0, BUFFER_SIZE);
            sprintf(buf, "PASS %s\r\n", password);

            printf("C> %s", buf);

            if (send(pop3_socket, buf, strlen(buf), 0) == -1) {
                perror("Send failed");
                close(pop3_socket);
                exit(EXIT_FAILURE);
            }

            memset(buf, 0, BUFFER_SIZE);
            recv(pop3_socket, buf, BUFFER_SIZE, 0);

            printf("S> %s", buf);

            tok = strtok(buf, " |\r\n");
            if(strcmp(tok, "+OK")!=0) {
                printf("Error in PASS, Password entered is invalid !!\n");

                printf("\nClick Enter to Continue ..\n");
                close(pop3_socket);
                continue;
            }

            login = 1;
            continue;
        }
        else{
            printf("\nChoose any Option : [Default 3]\n1. Manage Mails\n2. Send Mail\n3. Quit\n4. Logout\n\n");

            printf("Enter Choice : ");
            char ch[10];
            scanf("%s", ch);

            choice = atoi(ch);

            if(choice!=1 && choice!=2 && choice!=4){
                memset(buf, 0, BUFFER_SIZE);
                sprintf(buf, "QUIT\r\n");

                printf("C> %s", buf);

                if(send(pop3_socket, buf, strlen(buf), 0) == -1) {
                    perror("Send failed");
                    close(pop3_socket);
                    exit(EXIT_FAILURE);
                }

                memset(buf, 0, BUFFER_SIZE);
                recv(pop3_socket, buf, BUFFER_SIZE, 0);

                printf("S> %s", buf);

                close(pop3_socket);
                
                break;
            }
        }

        if(choice==4){
            login = 0;
            
            memset(buf, 0, BUFFER_SIZE);
            sprintf(buf, "QUIT\r\n");

            printf("C> %s", buf);

            if(send(pop3_socket, buf, strlen(buf), 0) == -1) {
                perror("Send failed");
                close(pop3_socket);
                exit(EXIT_FAILURE);
            }

            memset(buf, 0, BUFFER_SIZE);
            recv(pop3_socket, buf, BUFFER_SIZE, 0);

            printf("S> %s", buf);

            close(pop3_socket);

            continue;
        }
            
        if(choice==1){
            memset(buf, 0, BUFFER_SIZE);

            while(1){

                // STAT
                sprintf(buf, "STAT\r\n");

                printf("C> %s", buf);

                if (send(pop3_socket, buf, strlen(buf), 0) == -1) {
                    perror("Send failed");
                    close(pop3_socket);
                    exit(EXIT_FAILURE);
                }

                memset(buf, 0, BUFFER_SIZE);
                recv(pop3_socket, buf, BUFFER_SIZE, 0);

                printf("S> %s", buf);

                tok = strtok(buf, " |\r\n");

                if(strcmp(tok, "+OK")!=0) {
                    printf("Error in LIST\n");
                    close(pop3_socket);
                    continue;
                }

                int total_mails = atoi(strtok(NULL, " |\r\n"));

                total_mails = ntohl(total_mails);

                char mails[total_mails][MAX_LINES][BUFFER_SIZE];

                for(int i=1; i<=total_mails; i++){
                    memset(buf, 0, BUFFER_SIZE);

                    for(int j=0; j<MAX_LINES; j++) memset(mails[i-1][j], 0, BUFFER_SIZE);

                    // LIST

                    int ci = htonl(i);
                    sprintf(buf, "RETR %d\r\n", ci);

                    printf("C> %s", buf);

                    if (send(pop3_socket, buf, strlen(buf), 0) == -1) {
                        perror("Send failed");
                        close(pop3_socket);
                        exit(EXIT_FAILURE);
                    }

                    int line_no = 0;

                    char * tok;

                    char eod[6] = "-----";
                    int len;

                    while(1){
                        memset(buf, 0, BUFFER_SIZE);

                        recv(pop3_socket, buf, BUFFER_SIZE, 0);

                        len = strlen(buf);

                        char * temp = strdup(buf);

                        tok = strtok(temp, "\n|\0");

                        while(tok!=NULL){
                            strcat(mails[i-1][line_no], tok);
                            mails[i-1][line_no][strlen(mails[i-1][line_no])] = '\n';
                            tok = strtok(NULL, "\n|\0");
                            line_no++;
                        }

                        if(buf[len-1]!='\n') {
                            line_no--;
                            mails[i-1][line_no][strlen(mails[i-1][line_no])-1] = '\0';
                        }

                        if(len>=5) strncpy(eod, buf+len-5, 5);
                        else{
                            strncpy(eod, eod+len, 5-len);
                            strncpy(eod+5-len, buf, len);
                        }

                        if(strcmp(eod, "\r\n.\r\n") == 0) break;
                    }

                    // printf("Mail %d Received\n", i);

                    // for(int j=0; j<line_no; j++){
                    //     printf("%s", mails[i-1][j]);
                    // }
                }

                // printf("Got full mailbox !!\n");

                printf("\nS.No.\t");
                printf("%-30s\t", "From");
                printf("%-20s\t", "Date & Time");
                printf("Subject\n\n");

                for (int i = 0; i < total_mails; i++) {
                    // Extracting 'From' information
                    tok = strdup(mails[i][0] + 6);
                    tok[strlen(tok) - 2] = '\0'; // Removing '\n' from mails[i][0]
                    printf("%d\t%-30s\t", i + 1, tok);

                    // Extracting 'Date & Time' information
                    tok = strdup(mails[i][3] + 10);
                    tok[strlen(tok) - 2] = '\0'; // Removing '\n' from mails[i][3]
                    printf("%-20s\t", tok);

                    // Extracting 'Subject' information
                    tok = strdup(mails[i][2] + 9);
                    tok[strlen(tok) - 2] = '\0';
                    printf("%s\n", tok);
                }

                printf("\nEnter mail no. to see (-1 to Quit to Main Menu) : ");

                int mail_no;
                scanf("%d", &mail_no);

                if(mail_no==-1) break;

                if(mail_no<1 || mail_no>total_mails) {
                    printf("Invalid Mail No. !!\n");
                    continue;
                }

                printf("\n");

                if(strncmp(mails[mail_no-1][0], "DELETED", 7)==0) {
                    printf("Mail Number %d was DELETED !!\n", mail_no);
                }
                else{
                    for(int j=0; j<MAX_LINES; j++){
                        if(mails[mail_no-1][j][0]!=0) printf("%s", mails[mail_no-1][j]);
                    }
                }

                printf("\n\nPress 'd' to Delete or any other key to continue ..");

                getchar();
                char chr = getchar();

                if(chr=='d'){
                    memset(buf, 0, BUFFER_SIZE);

                    int cmno = htonl(mail_no);
                    sprintf(buf, "DELE %d\r\n", cmno);

                    printf("C> %s", buf);

                    if (send(pop3_socket, buf, strlen(buf), 0) == -1) {
                        perror("Send failed");
                        close(pop3_socket);
                        exit(EXIT_FAILURE);
                    }

                    memset(buf, 0, BUFFER_SIZE);
                    recv(pop3_socket, buf, BUFFER_SIZE, 0);

                    printf("S> %s", buf);

                    tok = strtok(buf, " |\r\n");

                    if(strcmp(tok, "+OK")!=0) {
                        printf("\nMail Number %d is already Marked to be DELETED !!\n\n", mail_no);
                    }
                }
            }


            continue;
        }

        // Create SMTP socket
        if ((smtp_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            perror("SMTP Socket creation failed");
            exit(EXIT_FAILURE);
        }

        // Connect to server
        if (connect(smtp_socket, (struct sockaddr *)&smtp_server_addr, sizeof(smtp_server_addr)) == -1) {
            perror("Connection failed");
            exit(EXIT_FAILURE);
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

            close(smtp_socket);
            continue;
        }

        i++;
        printf("\nUser entered %d lines\n\n", i);

        char * tmp = strdup(msg[0]);
        tok = strtok(tmp, " ");

        if(strcmp(tok, "From:")!=0) {
            printf("tok = %s\n", tok);
            printf("Error in From !!\n");
            close(smtp_socket);
            continue;
        }

        tok = strtok(NULL, " ");
        if(tok==NULL) {
            printf("NO Sender id Found !!\n");
            close(smtp_socket);
            continue;
        }

        char * sender_domain = strdup(tok);
        tok = strtok(tok, "@");

        if(strcmp(tok, username)!=0) {
            printf("You are logged in as %s, not as %s !!!\n", username, tok);
            close(smtp_socket);
            continue;
        }

        tmp = strdup(msg[1]);

        tok = strtok(tmp, " ");
        if(strcmp(tok, "To:")!=0) {
            printf("Error in To\n");
            close(smtp_socket);
            continue;
        }

        tok = strtok(NULL, " |\n");
        if(tok==NULL) {
            printf("NO Receiver id Found !!\n");
            close(smtp_socket);
            continue;
        }

        char * receiver_domain = strdup(tok);
        tmp = strdup(msg[2]);

        tok = strtok(tmp, " |\n");
        if(strcmp(tok, "Subject:")!=0) {
            printf("Error in Subject\n");
            close(smtp_socket);
            continue;
        }

        // memset(buf, 0, BUFFER_SIZE);

        // sprintf(buf, "<client connects to SMTP Port>\r\n");

        // printf("C> %s", buf);

        // if (send(smtp_socket, buf, strlen(buf), 0) == -1) {
        //     perror("Send failed");
        //     close(smtp_socket);
        //     exit(EXIT_FAILURE);
        // }

        memset(buf, 0, BUFFER_SIZE);
        recv(smtp_socket, buf, BUFFER_SIZE, 0);

        printf("S> %s", buf);

        tok = strtok(buf, " ");
        if(strcmp(tok, "220")!=0) {
            printf("Error in Connection\n");
            close(smtp_socket);
            continue;
        }

        memset(buf, 0, BUFFER_SIZE);
        tok = strtok(sender_domain, "@");
        tok = strtok(NULL, "@|\n");
        sprintf(buf, "HELO %s\r\n", tok);

        char * sender_domain_only = strdup(tok);

        printf("C> %s", buf);

        if(send(smtp_socket, buf, strlen(buf), 0) == -1) {
            perror("Send failed");
            close(smtp_socket);
            exit(EXIT_FAILURE);
        }

        memset(buf, 0, BUFFER_SIZE);
        recv(smtp_socket, buf, BUFFER_SIZE, 0);

        printf("S> %s", buf);

        tok = strtok(buf, " ");
        if(strcmp(tok, "250")!=0) {
            printf("Error in HELO\n");
            close(smtp_socket);
            continue;
        }

        memset(buf, 0, BUFFER_SIZE);
        sprintf(buf, "MAIL FROM: <%s@%s>\r\n", sender_domain, sender_domain_only);

        printf("C> %s", buf);

        if(send(smtp_socket, buf, strlen(buf), 0) == -1) {
            perror("Send failed");
            close(smtp_socket);
            exit(EXIT_FAILURE);
        }

        memset(buf, 0, BUFFER_SIZE);
        recv(smtp_socket, buf, BUFFER_SIZE, 0);

        printf("S> %s", buf);

        tok = strtok(buf, " ");
        if(strcmp(tok, "250")!=0) {
            printf("Error in MAIL FROM\n");
            close(smtp_socket);
            continue;
        }

        memset(buf, 0, BUFFER_SIZE);
        sprintf(buf, "RCPT TO: <%s>\r\n", receiver_domain);
        // sprintf(buf, "RCPT TO: %s\r\n", receiver_domain);

        printf("C> %s", buf);

        if(send(smtp_socket, buf, strlen(buf), 0) == -1) {
            perror("Send failed");
            close(smtp_socket);
            exit(EXIT_FAILURE);
        }

        memset(buf, 0, BUFFER_SIZE);
        recv(smtp_socket, buf, BUFFER_SIZE, 0);

        printf("S> %s", buf);

        tok = strtok(buf, " ");
        if(strcmp(tok, "250")!=0) {
            printf("Error in RCPT TO\n");
            close(smtp_socket);
            continue;
        }

        memset(buf, 0, BUFFER_SIZE);
        sprintf(buf, "DATA\r\n");

        printf("C> %s", buf);

        if(send(smtp_socket, buf, strlen(buf), 0) == -1) {
            perror("Send failed");
            close(smtp_socket);
            exit(EXIT_FAILURE);
        }

        memset(buf, 0, BUFFER_SIZE);
        recv(smtp_socket, buf, BUFFER_SIZE, 0);

        printf("S> %s", buf);

        tok = strtok(buf, " ");
        if(strcmp(tok, "354")!=0) {
            printf("Error in DATA\n");
            close(smtp_socket);
            continue;
        }

        // Send mail to server
        int j;
        for(j=0; j<i; j++) {
            // printf("C> %s\n", msg[j]);
            msg[j][strlen(msg[j])-1] = '\r';
            msg[j][strlen(msg[j])] = '\n';
            if (send(smtp_socket, msg[j], strlen(msg[j]), 0) == -1) {
                perror("Send failed");
                close(smtp_socket);
                exit(EXIT_FAILURE);
            }

            if(j==2){
                time ( &rawtime );
                timeinfo = localtime ( &rawtime );

                memset(buf, 0, BUFFER_SIZE);
                sprintf(buf, "Received: %d-%d-%d @%d:%d\r\n", timeinfo->tm_mday, timeinfo->tm_mon+1, timeinfo->tm_year+1900, timeinfo->tm_hour, timeinfo->tm_min);

                if (send(smtp_socket, buf, strlen(buf), 0) == -1) {
                    perror("Send failed");
                    close(smtp_socket);
                    exit(EXIT_FAILURE);
                }
            }
        }

        memset(buf, 0, BUFFER_SIZE);
        recv(smtp_socket, buf, BUFFER_SIZE, 0);

        printf("S> %s", buf);

        tok = strtok(buf, " ");
        if(strcmp(tok, "250")!=0) {
            printf("Error in DATA\n");
            close(smtp_socket);
            continue;
        }

        memset(buf, 0, BUFFER_SIZE);
        sprintf(buf, "QUIT\r\n");

        printf("C> %s", buf);

        if(send(smtp_socket, buf, strlen(buf), 0) == -1) {
            perror("Send failed");
            close(smtp_socket);
            exit(EXIT_FAILURE);
        }

        memset(buf, 0, BUFFER_SIZE);
        recv(smtp_socket, buf, BUFFER_SIZE, 0);

        printf("S> %s", buf);

        tok = strtok(buf, " ");

        if(strcmp(tok, "221")!=0) {
            printf("Error in QUIT\n");
            close(smtp_socket);
            continue;
        }

        // Close socket
        close(smtp_socket);
    }

    return 0;
}
