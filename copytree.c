// Name: Roei Mesilaty, ID: 315253336
#include "copytree.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

void create_path(const char *path) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s", path);
    for (char *p = buffer + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(buffer, S_IRWXU);
            *p = '/';
        }
    }
    mkdir(buffer, S_IRWXU);
}

void copy_file_content(const char *src, const char *dest) {
    char buffer[BUFSIZ];
    ssize_t bytes_read;
    int src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        perror("open failed");
        return;
    }
    int dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (dest_fd == -1) {
        perror("open failed");
        close(src_fd);
        return;
    }
    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        if (write(dest_fd, buffer, bytes_read) != bytes_read) {
            perror("write failed");
            close(src_fd);
            close(dest_fd);
            return;
        }
    }
    if (bytes_read == -1) {
        perror("read failed");
    }
    close(src_fd);
    close(dest_fd);
}

void copy_file(const char *src, const char *dest, int copy_symlinks, int copy_permissions) {
    struct stat file_stat;
    if (lstat(src, &file_stat) == -1) {
        perror("lstat failed");
        return;
    }

    if (S_ISLNK(file_stat.st_mode)) {
        if (copy_symlinks) {
            char link_target[256];
            ssize_t len = readlink(src, link_target, sizeof(link_target) - 1);
            if (len == -1) {
                perror("readlink failed");
                return;
            }
            link_target[len] = '\0';
            if (symlink(link_target, dest) == -1) {
                perror("symlink failed");
            }
        } else {
            copy_file_content(src, dest);
            if (copy_permissions) {
                if (chmod(dest, file_stat.st_mode) == -1) {
                    perror("chmod failed");
                }
            }
        }
    } else {
        copy_file_content(src, dest);
        if (copy_permissions) {
            if (chmod(dest, file_stat.st_mode) == -1) {
                perror("chmod failed");
            }
        }
    }
}

void copy_directory(const char *src, const char *dest, int copy_symlinks, int copy_permissions) {
    struct stat dir_stat;
    if (stat(src, &dir_stat) == -1) {
        perror("stat failed");
        return;
    }

    create_path(dest);

    if (copy_permissions) {
        if (chmod(dest, dir_stat.st_mode) == -1) {
            perror("chmod failed");
        }
    }

    DIR *dir = opendir(src);
    if (!dir) {
        perror("opendir failed");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[1024];
        char dest_path[1024];
        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest, entry->d_name);

        if (entry->d_type == DT_DIR) {
            copy_directory(src_path, dest_path, copy_symlinks, copy_permissions);
        } else {
            copy_file(src_path, dest_path, copy_symlinks, copy_permissions);
        }
    }
    closedir(dir);
}
