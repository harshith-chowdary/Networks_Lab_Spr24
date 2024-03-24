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

    int ret = m_bind(sockfd, "127.0.0.1", my_port, "127.0.0.1", op_port);
    if(ret < 0){
        printf("Error in binding\n");
        return 0;
    }

    printf("PID => %d\n", getpid());

    printf("Socket created and binded %d\n", sockfd);
    fflush(stdout);

    struct sockaddr_in src_addr;
    src_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(op_port);
    src_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    char buffer[MESSAGE_SIZE+1];
    memset(buffer, 0, MESSAGE_SIZE+1);

    socklen_t len = sizeof(src_addr);

    // for(int i = 0; i < 30; i++){
    //     ret = -1;
    //     while(ret < 0) {
    //         ret = m_recvfrom(sockfd, buffer, MESSAGE_SIZE, 0, (struct sockaddr *)&src_addr, &len);
    //     }

    //     printf("Received => %s\n", buffer);
    //     fflush(stdout);
    // }

    FILE *file = fopen("RECV1.txt", "w");

    int id = 1;

    while (1) {
        int ret = -1;
        while(ret < 0) {
            ret = m_recvfrom(sockfd, buffer, MESSAGE_SIZE+1, 0, (struct sockaddr *)&src_addr, &len);
        }

        if (strncmp(buffer, "END", 3) == 0) {
            // Uncomment this to print END on the local file
            // fprintf(file, "%s", buffer);
            printf("\nClosing local file.\n");
            break;
        }

        buffer[ret] = '\0';

        // printf("Received => %s\n", buffer);
        printf("Received %d => ", id++);
        fflush(stdout);
        fwrite(buffer, 1, ret, stdout);
        printf("\n\n");
        fflush(stdout);

        fprintf(file, "%s", buffer);
        // fwrite(buffer, 1, ret, file);

        memset(buffer, 0, MESSAGE_SIZE);
    }

    fclose(file);

    m_close(sockfd);

    return 0;
}