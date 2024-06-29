// Name: Roei Mesilaty, ID: 315253336
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

void write_message(const char *message, int count) {
    for (int i = 0; i < count; i++) {
        printf("%s\n", message);
        usleep((rand() % 100) * 1000); // Random delay between 0 and 99 milliseconds
    }
}

void getLockAccess(const char* lockfile) {
    int fd;
    while ((fd = open(lockfile, O_CREAT | O_EXCL, 0444)) == -1) {
        // If we get error that the file exists, it means that some other child currently has the lock, hence we need to wait until it's free.
        if (errno != EEXIST) {
            perror("Error acquiring lock");
            exit(1);
        }
        usleep(100000);
    }
}

void freeLock(const char* lockfile) {
    if (unlink(lockfile) == -1) {
        perror("Error releasing lock");
        exit(1);
    }
}

void main(int argc, char* argv[]) {
    if (argc < 6) {
        fprintf(stderr, "Usage: %s <message1> <message2> ... <order> <count>", argv[0]);
        return;
    }
    const char* lockfile = "lockfile.lock";
    const int numOfPrints = atoi(argv[argc - 1]);
    const int numOfChilds = argc - 2;

    pid_t pids[numOfChilds];

    for (int i = 0; i < numOfChilds; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork failed");
            exit(1);
        }
        else if (pids[i] == 0) {
            getLockAccess(lockfile);
            write_message(argv[i + 1], numOfPrints);
            freeLock(lockfile);
            exit(0);
        }
    }
    for (int i = 0; i < numOfChilds; i++) {
        wait(NULL);
    }
    return;
}