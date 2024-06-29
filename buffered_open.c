#include "buffered_open.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

// Helper function to allocate and initialize a buffered_file_t structure
static buffered_file_t* init_buffered_file(int fd, int flags) {
    buffered_file_t *bf = (buffered_file_t *)malloc(sizeof(buffered_file_t));
    if (!bf) return NULL;

    bf->fd = fd;
    bf->read_buffer = (char *)malloc(BUFFER_SIZE);
    bf->write_buffer = (char *)malloc(BUFFER_SIZE);
    if (!bf->read_buffer || !bf->write_buffer) {
        free(bf->read_buffer);
        free(bf->write_buffer);
        free(bf);
        return NULL;
    }

    bf->read_buffer_size = BUFFER_SIZE;
    bf->write_buffer_size = BUFFER_SIZE;
    bf->read_buffer_pos = 0;
    bf->write_buffer_pos = 0;
    bf->flags = flags;
    bf->preappend = (flags & O_PREAPPEND) ? 1 : 0;

    return bf;
}

buffered_file_t* buffered_open(const char *pathname, int flags, ...) {
    int fd;
    va_list args;
    va_start(args, flags);

    // Remove the O_PREAPPEND flag for the actual open call
    int open_flags = flags & ~O_PREAPPEND;
    if (flags & O_CREAT) {
        mode_t mode = va_arg(args, mode_t);
        fd = open(pathname, open_flags, mode);
    } else {
        fd = open(pathname, open_flags);
    }
    va_end(args);

    if (fd == -1) return NULL;

    return init_buffered_file(fd, flags);
}

ssize_t buffered_write(buffered_file_t *bf, const void *buf, size_t count) {
    // Handle O_PREAPPEND logic
    if (bf->preappend) {
        // Read the existing file content from the current position to the end
        off_t current_pos = lseek(bf->fd, 0, SEEK_CUR);
        off_t end_pos = lseek(bf->fd, 0, SEEK_END);
        size_t remaining_size = end_pos - current_pos;

        // Allocate a temporary buffer to hold the remaining content
        char *temp_buffer = (char *)malloc(remaining_size);
        if (!temp_buffer) return -1;

        // Read the remaining content into the temporary buffer
        lseek(bf->fd, current_pos, SEEK_SET);
        read(bf->fd, temp_buffer, remaining_size);

        // Write the new data at the current position
        lseek(bf->fd, current_pos, SEEK_SET);
        write(bf->fd, buf, count);

        // Append the remaining content after the new data
        write(bf->fd, temp_buffer, remaining_size);
        lseek(bf->fd,0,SEEK_SET);
        // Clean up and reset the flag
        free(temp_buffer);

        return count;
    }

    // // If the last operation was reading, adjust the file descriptor position
    // if (bf->read_buffer_pos > 0) {
    //     off_t read_offset = bf->read_buffer_pos - bf->read_buffer_size;
    //     if (lseek(bf->fd, read_offset, SEEK_CUR) == -1) {
    //         return -1;
    //     }
    //     bf->read_buffer_pos = 0;
    // }

    // Write to the write buffer
    size_t total_written = 0;
    while (count > 0) {
        size_t space_left = bf->write_buffer_size - bf->write_buffer_pos;
        size_t to_write = count < space_left ? count : space_left;

        memcpy(bf->write_buffer + bf->write_buffer_pos, buf, to_write);
        bf->write_buffer_pos += to_write;
        buf = (const char *)buf + to_write;
        count -= to_write;
        total_written += to_write;

        if (bf->write_buffer_pos == bf->write_buffer_size) {
            if (buffered_flush(bf) == -1) return -1;
        }
    }
    return total_written;
}

ssize_t buffered_read(buffered_file_t *bf, void *buf, size_t count) {
    // Flush the write buffer before reading
    if (buffered_flush(bf) == -1) return -1;

    size_t total_read = 0;
    while (count > 0) {
        if (bf->read_buffer_pos == 0) {
            ssize_t bytes_read = read(bf->fd, bf->read_buffer, BUFFER_SIZE);
            if (bytes_read <= 0) return total_read ? total_read : bytes_read;
            bf->read_buffer_size = bytes_read;
        }

        size_t to_read = bf->read_buffer_size - bf->read_buffer_pos;
        if (to_read > count) to_read = count;

        memcpy(buf, bf->read_buffer + bf->read_buffer_pos, to_read);
        bf->read_buffer_pos += to_read;
        buf = (char *)buf + to_read;
        count -= to_read;
        total_read += to_read;

        if (bf->read_buffer_pos == bf->read_buffer_size) bf->read_buffer_pos = 0;
    }

    // Adjust the file descriptor to point after the actual bytes read
    off_t current_pos = lseek(bf->fd, total_read, SEEK_CUR);
    if (current_pos == -1) {
        perror("lseek");
        return -1;
    }

    return total_read;
}


int buffered_flush(buffered_file_t *bf) {
    if (bf->write_buffer_pos > 0) {
        ssize_t written = write(bf->fd, bf->write_buffer, bf->write_buffer_pos);
        if (written == -1) return -1;
        bf->write_buffer_pos = 0;
    }
    return 0;
}

int buffered_close(buffered_file_t *bf) {
    if (buffered_flush(bf) == -1) return -1;
    int result = close(bf->fd);

    free(bf->read_buffer);
    free(bf->write_buffer);
    free(bf);
    return result;
}