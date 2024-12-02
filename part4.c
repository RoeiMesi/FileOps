#include "copytree.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void usage(const char *prog_name) {
    fprintf(stderr, "Usage: %s [-l] [-p] <source_directory> <destination_directory>\n", prog_name);
    fprintf(stderr, "  -l: Preserve symbolic links\n");
    fprintf(stderr, "  -p: Preserve file permissions\n");
}

int main(int argc, char *argv[]) {
    int opt;
    int symlink_flag = 0;
    int permissions_flag = 0;

    while ((opt = getopt(argc, argv, "lp")) != -1) {
        switch (opt) {
            case 'l':
                symlink_flag = 1;
                break;
            case 'p':
                permissions_flag = 1;
                break;
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (optind + 2 != argc) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *src_dir = argv[optind];
    const char *dest_dir = argv[optind + 1];

    copy_directory(src_dir, dest_dir, symlink_flag, permissions_flag);

    return 0;
}
