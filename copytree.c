// copytree.c
#include "copytree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

// Helper function to create directories recursively
void create_directories(const char *path) {
    char temp[256];
    snprintf(temp, sizeof(temp), "%s", path);
    for (char *p = temp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            mkdir(temp, S_IRWXU);
            *p = '/';
        }
    }
    mkdir(temp, S_IRWXU);
}

void copy_file(const char *src, const char *dest, int copy_symlinks, int copy_permissions) {
    struct stat statbuf;
    if (lstat(src, &statbuf) == -1) {
        perror("lstat failed");
        return;
    }

    if (S_ISLNK(statbuf.st_mode)) {
        if (copy_symlinks) {
            char link_target[256];
            ssize_t len = readlink(src, link_target, sizeof(link_target)-1);
            if (len == -1) {
                perror("readlink failed");
                return;
            }
            link_target[len] = '\0';
            if (symlink(link_target, dest) == -1) {
                perror("symlink failed");
            }
        } else {
            char buf[BUFSIZ];
            ssize_t nread;
            int src_fd = open(src, O_RDONLY);
            if (src_fd == -1) {
                perror("open failed");
                return;
            }
            int dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, statbuf.st_mode);
            if (dest_fd == -1) {
                perror("open failed");
                close(src_fd);
                return;
            }
            while ((nread = read(src_fd, buf, sizeof(buf))) > 0) {
                if (write(dest_fd, buf, nread) != nread) {
                    perror("write failed");
                    close(src_fd);
                    close(dest_fd);
                    return;
                }
            }
            if (nread == -1) {
                perror("read failed");
            }
            close(src_fd);
            close(dest_fd);
        }
    } else {
        char buf[BUFSIZ];
        ssize_t nread;
        int src_fd = open(src, O_RDONLY);
        if (src_fd == -1) {
            perror("open failed");
            return;
        }
        int dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, statbuf.st_mode);
        if (dest_fd == -1) {
            perror("open failed");
            close(src_fd);
            return;
        }
        while ((nread = read(src_fd, buf, sizeof(buf))) > 0) {
            if (write(dest_fd, buf, nread) != nread) {
                perror("write failed");
                close(src_fd);
                close(dest_fd);
                return;
            }
        }
        if (nread == -1) {
            perror("read failed");
        }
        if (copy_permissions) {
            if (fchmod(dest_fd, statbuf.st_mode) == -1) {
                perror("fchmod failed");
            }
        }
        close(src_fd);
        close(dest_fd);
    }
}

void copy_directory(const char *src, const char *dest, int copy_symlinks, int copy_permissions) {
    struct stat statbuf;
    if (stat(src, &statbuf) == -1) {
        perror("stat failed");
        return;
    }

    create_directories(dest);

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
