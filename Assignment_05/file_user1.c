#include <msocket.h>

int main(int argc, char * argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int my_port = atoi(argv[1]);
    int op_port = atoi(argv[2]);

    int sockfd = m_socket(AF_INET, SOCK_MTP, 0);
    if(sockfd < 0){
        printf("Error in creating socket\n");
        printf("%d\n", errno);
        return 0;
    }

    printf("PID => %d\n", getpid());
    
    printf("Socket created and binded %d\n", sockfd);
    fflush(stdout);

    int ret = m_bind(sockfd, "127.0.0.1", my_port, "127.0.0.1", op_port);
    if(ret < 0){
        printf("Error in binding\n");
        return 0;
    }

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(op_port);
    dest_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    char buffer[MESSAGE_SIZE];
    memset(buffer, 0, MESSAGE_SIZE);

    socklen_t len = sizeof(dest_addr);

    // for(int i = 0; i < 30; i++){
    //     sprintf(buffer, "Message %d", i+1);

    //     ret = -1;
    //     while(ret < 0) {
    //         ret = m_sendto(sockfd, buffer, MESSAGE_SIZE, 0, (struct sockaddr *)&dest_addr, len);
    //     }

    //     printf("Sent => %s\n", buffer);
    //     fflush(stdout);
    // }

    char * filename = "Sample100.txt";
    // char * filename = "test.txt";

    FILE *file = fopen(filename, "r");

    // while (fscanf(file, "%1023s", buffer) == 1) {
    //     char nextChar = fgetc(file);
    //     int temp = strlen(buffer);

    //     printf("Sending %s\n", buffer);

    //     while(nextChar == 10 || nextChar == 32){
    //         buffer[temp++] = nextChar;
    //         nextChar = fgetc(file);
    //     }
        
    //     if(nextChar != EOF) {
    //         ungetc(nextChar, file);
    //     }

    //     ret = -1;
    //     while(ret < 0) {
    //         ret = m_sendto(sockfd, buffer, MESSAGE_SIZE, 0, (struct sockaddr *)&dest_addr, len);
    //     }

    //     memset(buffer, 0, sizeof(buffer));
    // }

    // Read 1KB at a time until EOF
    size_t bytesRead;

    int total_messages = 0;
    while ((bytesRead = fread(buffer, 1, MESSAGE_SIZE, file)) > 0) {
        // Process the buffer (print it as an example)

        fwrite(buffer, 1, bytesRead, stdout);

        ret = -1;
        while(ret < 0) {
            ret = m_sendto(sockfd, buffer, bytesRead, 0, (const struct sockaddr*)&dest_addr, len);
        }

        total_messages++;

        // buffer[bytesRead] = '\0';
        // printf("Sent => %s\n", buffer);

        printf("Sent => ");
        fflush(stdout);
        fwrite(buffer, 1, bytesRead, stdout);
        printf("\n\n");
        fflush(stdout);

        memset(buffer, 0, MESSAGE_SIZE);
    }

    printf("\n\n\n\n\n");

    printf("Total messages sent: %d\n", total_messages);

    ret = -1;
    while(ret < 0) {
        ret = m_sendto(sockfd, "END", 3, 0, (const struct sockaddr*)&dest_addr, len);
    }

    fclose(file);

    sleep(10);

    // m_close(sockfd);
    
    return 0;
}