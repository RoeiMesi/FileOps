#include "buffered_open.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>

#define BUFFER_SIZE 4096

// Helper function to allocate and initialize a buffered_file_t structure
size_t min(size_t a, size_t b) {
    if (a < b)
        return a;
    return b;
}

buffered_file_t *buffered_open(const char *pathname, int flags, ...) {
    buffered_file_t *bf = (buffered_file_t *)malloc(sizeof(buffered_file_t));
    if (!bf) {
        perror("Failed to allocate memory for buffered_file_t");
        return NULL;
    }

    bf->read_buffer = (char *)malloc(BUFFER_SIZE);
    bf->write_buffer = (char *)malloc(BUFFER_SIZE);
    if (!bf->read_buffer || !bf->write_buffer) {
        perror("Failed to allocate buffers");
        free(bf->read_buffer);
        free(bf->write_buffer);
        free(bf);
        return NULL;
    }

    bf->read_buffer_size = 0;
    bf->write_buffer_size = BUFFER_SIZE;
    bf->read_buffer_pos = 0;
    bf->write_buffer_pos = 0;
    if (flags & O_PREAPPEND) {
        bf->preappend = 1;
    } else
        bf->preappend = 0;
    bf->flags = flags;

    // Remove O_PREAPPEND before calling the original open function
    flags &= ~O_PREAPPEND;

    va_list args;
    va_start(args, flags);
    if (flags & O_CREAT) {
        mode_t mode = va_arg(args, int);
        bf->fd = open(pathname, flags, mode);
    } else {
        bf->fd = open(pathname, flags);
    }
    va_end(args);

    if (bf->fd == -1) {
        perror("Failed to open file");
        free(bf->read_buffer);
        free(bf->write_buffer);
        free(bf);
        return NULL;
    }
    return bf;
}

ssize_t buffered_write(buffered_file_t *bf, const void *buf, size_t count) {
    if (bf->flags == O_RDONLY) {
        return -1;
    }
    size_t bytes_to_write = count;
    size_t buffer_space = bf->write_buffer_size - bf->write_buffer_pos;

    while (bytes_to_write > buffer_space) {
        // Copy data to buffer
        memcpy(bf->write_buffer + bf->write_buffer_pos, buf, buffer_space);
        bf->write_buffer_pos = bf->write_buffer_size;

        // Flush the buffer (flushing the buffer makes bf->write_buffer_pos = 0)
        if (buffered_flush(bf) == -1) {
            return -1;
        }

        // Now the buffer is empty
        buffer_space = bf->write_buffer_size;

        // Update buf to point to the next section to write.
        buf += buffer_space;
        bytes_to_write -= buffer_space;
    }

    // Copy the remaining data to buffer
    memcpy(bf->write_buffer + bf->write_buffer_pos, buf, bytes_to_write);
    bf->write_buffer_pos += bytes_to_write;
    return (ssize_t) count;
}


ssize_t buffered_read(buffered_file_t *bf, void *buf, size_t count) {
    if (bf->flags & O_WRONLY){
        return -1;
    }
    size_t bytes_to_read = count;   // maximum bytes to read
    size_t buffer_space = bf->read_buffer_size - bf->read_buffer_pos;

    while (buffer_space < bytes_to_read) {
        // Copy data from buffer
        memcpy(buf, bf->read_buffer + bf->read_buffer_pos, buffer_space);
        bytes_to_read -= buffer_space;
        buf += buffer_space;

        // Load read_buffer with new data
        bf->read_buffer_pos = 0;
        bf->read_buffer_size = read(bf->fd, bf->read_buffer, BUFFER_SIZE);
        if (bf->read_buffer_size == -1)
            perror("Failed to read from file");
        buffer_space = bf->read_buffer_size;

        // No more to read from the file
        if (bf->read_buffer_size < BUFFER_SIZE) {
            break;
        }
    }

    size_t remain_bytes_to_read = min(buffer_space, bytes_to_read);
    memcpy(buf, bf->read_buffer + bf->read_buffer_pos, remain_bytes_to_read);
    bf->read_buffer_pos += remain_bytes_to_read;
    return (ssize_t) count - bytes_to_read + remain_bytes_to_read; // bytes_read
}

int flush_pre_append(buffered_file_t *bf) {
    if (bf->write_buffer_pos == 0)
        return 0;

    // Get the number of bytes to flush.
    size_t buf_size = bf->write_buffer_pos;

    // Save current position
    off_t current_pos = lseek(bf->fd, 0, SEEK_CUR);

    // Create temp txt file
    int temp_fd = open("temp_file.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (temp_fd == -1) {
        perror("Failed to open temporary file");
        return -1;
    }

    // Flush the buffer to that file
    ssize_t written_bytes = write(temp_fd, bf->write_buffer, buf_size);
    if (written_bytes == -1) {
        perror("Failed to write buffer content to temporary file");
        close(temp_fd);
        return -1;
    }

    // Reset buffer position
    bf->write_buffer_pos = 0;

    // Copy from bf->fd to temp_fd
    char buffer[BUFFER_SIZE];
    ssize_t read_bytes;
    while ((read_bytes = read(bf->fd, buffer, BUFFER_SIZE)) > 0) {
        written_bytes = write(temp_fd, buffer, read_bytes);
        if (written_bytes == -1) {
            perror("Failed to write existing data to temporary file");
            close(temp_fd);
            return -1;
        }
    }

    if (read_bytes == -1) {
        perror("Failed to read existing data from original file");
        close(temp_fd);
        return -1;
    }

    // Go back to current_pos
    lseek(bf->fd, current_pos, SEEK_SET);

    // Copy from temp_fd to bf->fd
    lseek(temp_fd, 0, SEEK_SET); // Move to the start of the temp file
    while ((read_bytes = read(temp_fd, buffer, BUFFER_SIZE)) > 0) {
        written_bytes = write(bf->fd, buffer, read_bytes);
        if (written_bytes == -1) {
            perror("Failed to write back to the original file");
            close(temp_fd);
            return -1;
        }
    }

    if (read_bytes == -1) {
        perror("Failed to read from temporary file");
        close(temp_fd);
        return -1;
    }

    // Close and delete temp_fd
    close(temp_fd);
    remove("temp_file.txt");

    // Get to the current position
    lseek(bf->fd, current_pos + buf_size, SEEK_SET);
    return 0;
}


int buffered_flush(buffered_file_t *bf) {
    if (bf->preappend) {
        return flush_pre_append(bf);
    }

    if (bf->write_buffer_pos != 0){
        ssize_t written_bytes = write(bf->fd, bf->write_buffer, bf->write_buffer_pos);
        if (written_bytes == -1) {
            perror("Failed to write to file");
            return -1;
        }
        bf->write_buffer_pos = 0; // Reset the buffer position
    }
    return 0;
}


int buffered_close(buffered_file_t *bf) {
    if (buffered_flush(bf) == -1) {
        perror("Failed to flush before closing");
        return -1;
    }

    if (close(bf->fd) == -1) {
        perror("Failed to close file");
        return -1;
    }

    free(bf->read_buffer);
    free(bf->write_buffer);
    free(bf);
    return 0;
}
