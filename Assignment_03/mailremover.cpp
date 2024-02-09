#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_LINE_LENGTH 1000

int main() {
    FILE *file = fopen("mailbox.txt", "r+");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    bool insideMail = false;
    int mailCount = 0;
    int n = 2; // Specify the nth mail to be removed
    char line[MAX_LINE_LENGTH];

    // Rewind the file pointer to the beginning
    rewind(file);

    while (fgets(line, sizeof(line), file)) {
        // Check for the start of a mail message
        if (strncmp(line, "From:", 5) == 0) {
            insideMail = true;
            mailCount++;
            // Skip the first n mail messages
            if (mailCount <= n) {
                while (strncmp(line, ".", 2) != 0) {
                    fgets(line, sizeof(line), file);
                }
                continue;
            }
        }
        // If not inside the first n mail messages, move the content to the beginning of the file
        if (!insideMail) {
            fseek(file, 0, SEEK_CUR); // Move the file pointer
            fputs(line, file);
        }
        // Check for the end of a mail message
        if (strncmp(line, ".", 2) == 0) {
            insideMail = false;
        }
    }

    // Truncate the file to remove any remaining content from the first n mail messages
    if (ftruncate(fileno(file), ftell(file)) != 0) {
        perror("Error truncating file");
        exit(EXIT_FAILURE);
    }

    fclose(file);

    printf("%dth mail removed successfully.\n", n);

    return 0;
}
