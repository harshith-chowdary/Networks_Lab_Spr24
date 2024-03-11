#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_IP_LENGTH 256

int main() {
    char ip_address[MAX_IP_LENGTH];

    // Open a pipe to run the hostname -I command
    FILE *pipe = popen("hostname -I", "r");
    if (pipe == NULL) {
        perror("Failed to open pipe");
        exit(EXIT_FAILURE);
    }

    // Read the output of the command
    if (fgets(ip_address, MAX_IP_LENGTH, pipe) == NULL) {
        perror("Failed to read from pipe");
        exit(EXIT_FAILURE);
    }

    // Close the pipe
    if (pclose(pipe) == -1) {
        perror("Failed to close pipe");
        exit(EXIT_FAILURE);
    }

    // Trim newline character if present
    size_t len = strlen(ip_address);
    if (len > 0 && ip_address[len - 1] == '\n') {
        ip_address[len - 1] = '\0';
    }

    printf("IP Address: %s\n", ip_address);

    return 0;
}
