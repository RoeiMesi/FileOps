// Name: Roei Mesilaty - ID: 315253336
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void writeToFile(const char* filename, char* message, int amount) {
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Failed to open file.\n");
        exit(1);
    }
    for (int i = 0; i < amount; i++) {
        fprintf(file, "%s\n", message);
    }
    fclose(file);
}

int main(int argc, char* argv[]) {
    pid_t pid1, pid2;
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <parent_message> <child1_message> <child2_message> <count>\n", argv[0]);
        return 1;
    }
    else {
        pid1 = fork();
        if (pid1 < 0) {
            perror("fork failed");
            exit(1);
        }
        else if (pid1 == 0) {
            /* Child Process*/
            writeToFile("output.txt", argv[2], atoi(argv[4]));
            exit(0);
        }
        pid2 = fork();
        if (pid2 < 0) {
            perror("fork failed");
            exit(1);
        }
        else if (pid2 == 0) {
            // Added sleep to make sure child1 prints his messages before child2.
            sleep(1);
            writeToFile("output.txt", argv[3], atoi(argv[4]));
            exit(0);
        }
    }
    wait(NULL);
    wait(NULL);
    writeToFile("output.txt", argv[1],atoi(argv[4]));

    return 0;
}