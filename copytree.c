// Name: Roei Mesilaty, ID: 315253336
#include "copytree.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/limits.h>

// Function to manage the copying of symbolic links
void copy_symlink(const char *src, const char *dst) {
    char link_target[PATH_MAX];
    ssize_t target_length = readlink(src, link_target, sizeof(link_target) - 1);
    if (target_length == -1) {
        perror("readlink");
        return;
    }
    link_target[target_length] = '\0';
    if (symlink(link_target, dst) == -1) {
        perror("symlink");
    }
}

// Function to handle regular file copying
void copy_file(const char *src, const char *dest, int copy_symlinks, int copy_permissions) {
    int source_fd = open(src, O_RDONLY);
    if (source_fd == -1) {
        perror("open source");
        return;
    }

    struct stat file_info;
    if (fstat(source_fd, &file_info) == -1) {
        perror("fstat");
        close(source_fd);
        return;
    }

    int dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, file_info.st_mode);
    if (dest_fd == -1) {
        perror("open destination");
        close(source_fd);
        return;
    }

    char buffer[BUFSIZ];
    ssize_t bytes_transferred;
    while ((bytes_transferred = read(source_fd, buffer, sizeof(buffer))) > 0) {
        if (write(dest_fd, buffer, bytes_transferred) != bytes_transferred) {
            perror("write");
            close(source_fd);
            close(dest_fd);
            return;
        }
    }
    if (bytes_transferred == -1) {
        perror("read");
    }

    close(source_fd);
    close(dest_fd);

    if (copy_permissions) {
        if (chmod(dest, file_info.st_mode) == -1) {
            perror("chmod");
        }
    }
}

void transfer_file(const char *src, const char *dest, int copy_symlinks, int copy_permissions) {
    struct stat file_info;
    if (lstat(src, &file_info) == -1) {
        perror("lstat");
        return;
    }

    if (S_ISLNK(file_info.st_mode) && copy_symlinks) {
        copy_symlink(src, dest);
    } else if (S_ISREG(file_info.st_mode)) {
        copy_file(src, dest, copy_symlinks, copy_permissions);
    }
}

void create_directories_recursive(const char *path) {
    char temp_path[PATH_MAX];
    char *sub_path = NULL;
    size_t length;

    snprintf(temp_path, sizeof(temp_path), "%s", path);
    length = strlen(temp_path);
    if (temp_path[length - 1] == '/') {
        temp_path[length - 1] = '\0';
    }
    for (sub_path = temp_path + 1; *sub_path; sub_path++) {
        if (*sub_path == '/') {
            *sub_path = '\0';
            mkdir(temp_path, S_IRWXU);
            *sub_path = '/';
        }
    }
    mkdir(temp_path, S_IRWXU);
}

// Function to handle directory content copying
void process_directory_contents(const char *src, const char *dest, int copy_symlinks, int copy_permissions) {
    DIR *source_dir = opendir(src);
    if (!source_dir) {
        perror("opendir");
        return;
    }

    create_directories_recursive(dest);

    struct dirent *dir_entry;
    while ((dir_entry = readdir(source_dir)) != NULL) {
        if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0) {
            continue;
        }

        char src_entry_path[PATH_MAX];
        char dest_entry_path[PATH_MAX];
        snprintf(src_entry_path, sizeof(src_entry_path), "%s/%s", src, dir_entry->d_name);
        snprintf(dest_entry_path, sizeof(dest_entry_path), "%s/%s", dest, dir_entry->d_name);

        struct stat entry_info;
        if (lstat(src_entry_path, &entry_info) == -1) {
            perror("lstat");
            continue;
        }

        if (S_ISDIR(entry_info.st_mode)) {
            copy_directory(src_entry_path, dest_entry_path, copy_symlinks, copy_permissions);
        } else {
            transfer_file(src_entry_path, dest_entry_path, copy_symlinks, copy_permissions);
        }
    }
    closedir(source_dir);
}

void copy_directory(const char *src, const char *dest, int copy_symlinks, int copy_permissions) {
    process_directory_contents(src, dest, copy_symlinks, copy_permissions);
}
