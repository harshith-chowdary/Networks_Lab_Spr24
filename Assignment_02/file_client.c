#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <dirent.h>
#include <fcntl.h>

#define MAX 100

void flushInput() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        // Read and discard characters
    }
}

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
        char path[MAX];
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

int main(){
    int sockfd;
    struct sockaddr_in serv_addr;

    printf("Client started !!\n");

    int i;
    char buf[MAX];

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Unable to create socket !!\n");
        exit(0);
    }

    serv_addr.sin_family = AF_INET;
    // inet_aton("127.0.0.1", &serv_addr.sin_addr);
    inet_aton("10.117.166.16", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(20000);

    if((connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0){
        perror("Unable to connect to the server !!\n");
        exit(0);
    }

    printf("Connected to the server !!\n");

    memset(buf, 0, MAX);

    recv(sockfd, buf, MAX, 0);

    printf("%s\n", buf);

    sprintf(buf, "Message from client");
    send(sockfd, buf, strlen(buf)+1, 0);

    int file_desc, k;
    char filename[MAX];

    while(1){
        memset(filename, 0, MAX);

        printf("\nEnter the filename name : ");
        fgets(filename, MAX - 1, stdin);
        filename[strlen(filename) - 1] = '\0';

        printf("Enter the value for k : ");
        scanf("%d", &k);

        memset(buf, 0, MAX);
        sprintf(buf, "%d", k);

        printf("File Name : %s & k : %d\n", filename, k);

        send(sockfd, buf, strlen(buf) + 1, 0);

        file_desc = open(searchFile(filename, "."), O_RDONLY);

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

            send(sockfd, buf, MAX, 0);

            if(bytes_read < MAX) break;

            memset(buf, 0, MAX); // ASCII value of '?'
        }

        printf("File sent successfully from Client !!\n");
        close(file_desc);

        // To receive the encrypted file from the server

        memset(buf, 0, MAX);
        strcpy(buf, filename);
        buf[strlen(buf)] = '.';
        strcat(buf, "enc");

        printf("Encrypted File Name : %s\n", buf);

        file_desc = open(buf, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

        if (file_desc == -1)
        {
            perror("Error opening file");
            return 1;
            exit(0);
        }

        // Storing Final Encrypted File from the server

        while(1){
            memset(buf, 0, MAX);

            int len;
            len = recv(sockfd, buf, MAX, 0);

            if(buf[0] == 0) break;

            int end = 0;
            while(end < len && buf[end] != 0 && buf[end] != 64) end++;

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

        close(file_desc);

        printf("Done !!!\n");
        printf("Actual File    : %s\n", filename);
        printf("Encrypted File : %s.enc\n", filename);

        printf("\nDo you want to continue ? (1/0) : ");
        scanf("%d", &i);
        if(!i) break;

        flushInput();

        send(sockfd, "1\0", 2, 0);
    }

    close(sockfd);
    return 0;
}