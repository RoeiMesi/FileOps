#include "copytree.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void display_usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s [-l] [-p] <source_directory> <destination_directory>\n", prog_name);
    fprintf(stderr, "  -l: Copy symbolic links as links\n");
    fprintf(stderr, "  -p: Preserve file permissions\n");
}

int main(int argc, char *argv[]) {
    int option;
    int symlink_flag = 0;
    int permission_flag = 0;

    while ((option = getopt(argc, argv, "lp")) != -1) {
        switch (option) {
            case 'l':
                symlink_flag = 1;
                break;
            case 'p':
                permission_flag = 1;
                break;
            default:
                display_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (optind + 2 != argc) {
        display_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *source_dir = argv[optind];
    const char *destination_dir = argv[optind + 1];

    copy_directory(source_dir, destination_dir, symlink_flag, permission_flag);

    return 0;
}
