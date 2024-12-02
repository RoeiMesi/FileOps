#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void writeToFile(const char *filename, char* message, int amount) {
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
    chdir("../");
    pid_t pid1, pid2;
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <parent_message> <child1_message> <child2_message> <count>\n", argv[0]);
        return 1;
    }
    const char *filename = "output.txt";

    // Clear the text file from any previous content using the write mode.
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Failed to open file.\n");
        exit(1);
    }

    // Fork first child
    pid1 = fork();
    if (pid1 < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid1 == 0) {
        /* First child */
        sleep(1);
        writeToFile(filename, argv[2], atoi(argv[4]));
        exit(0);
    }

    // Fork second child
    pid2 = fork();
    if (pid2 < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid2 == 0) {
        /* Second child */
        sleep(2);
        writeToFile(filename, argv[3], atoi(argv[4]));
        exit(0);
    }

    // Parent process, wait for both children to finish
    wait(NULL);
    wait(NULL);

    fclose(file);
    writeToFile(filename, argv[1], atoi(argv[4]));

    return 0;
}
