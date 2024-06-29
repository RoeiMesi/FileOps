#include "buffered_open.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <math.h>
#define CHUNK_SIZE 1024

#define TEST_FILE "test_file.txt"
#define BUFFER_SIZE 4096

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
        // flags = O_RDWR;  // open file with O_RDWR and force manually flags.
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

    printf("File opened successfully: fd = %d\n", bf->fd);
    return bf;
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

ssize_t buffered_write(buffered_file_t *bf, const void *buf, size_t count) {
    size_t bytes_to_write = count;
    size_t buffer_space = bf->write_buffer_size - bf->write_buffer_pos;

    while (bytes_to_write > buffer_space) {
        // Copy data to buffer
        memcpy(bf->write_buffer + bf->write_buffer_pos, buf, buffer_space);

        // Flush the buffer (flushing the buffer makes bf->write_buffer_pos = 0)
        if (buffered_flush(bf) == -1) {
            return -1;
        }

        // now the buffer is empty
        buffer_space = bf->write_buffer_size;

        // update buf to point to the next section to write.
        buf += buffer_space;
        bytes_to_write -= buffer_space;
    }

    // Copy the remaining data to buffer
    memcpy(bf->write_buffer + bf->write_buffer_pos, buf, bytes_to_write);
    bf->write_buffer_pos += bytes_to_write;
    return (ssize_t) count;
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
        if ((bf->flags & O_RDONLY) || (bf->flags & O_WRONLY))
            return -1;
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

void test_open_and_write() {
    // Data to write
    const char *data = "Hello World!";
    size_t data_len = strlen(data);

    // Use regular functions
    int fd = open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(fd != -1);
    ssize_t bytes_written = write(fd, data, data_len);
    assert(bytes_written == data_len);
    close(fd);

    // Use buffered functions
    buffered_file_t *bf = buffered_open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(bf != NULL);
    ssize_t buffered_bytes_written = buffered_write(bf, data, data_len);
    assert(buffered_bytes_written == data_len);
    buffered_close(bf);

    // Compare file contents
    FILE *f = fopen(TEST_FILE, "r");
    assert(f != NULL);
    char buffer[BUFFER_SIZE];
    size_t bytes_read = fread(buffer, 1, BUFFER_SIZE, f);
    fclose(f);

    // Assert the contents are the same
    assert(bytes_read == data_len);
    assert(memcmp(buffer, data, data_len) == 0);

    printf("test_open_and_write passed\n");
}

void test_read() {
    // Data to write
    const char *data = "This is a test for reading.";
    size_t data_len = strlen(data);

    // Write data to file
    int fd = open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(fd != -1);
    write(fd, data, data_len);
    close(fd);

    // Read using regular functions
    fd = open(TEST_FILE, O_RDONLY);
    assert(fd != -1);
    char regular_buffer[BUFFER_SIZE];
    ssize_t regular_bytes_read = read(fd, regular_buffer, BUFFER_SIZE);
    close(fd);

    // Read using buffered functions
    buffered_file_t *bf = buffered_open(TEST_FILE, O_RDONLY);
    assert(bf != NULL);
    char buffered_buffer[BUFFER_SIZE];
    ssize_t buffered_bytes_read = buffered_read(bf, buffered_buffer, BUFFER_SIZE);
    buffered_close(bf);

    // Compare results
    assert(buffered_bytes_read == regular_bytes_read);
    assert(memcmp(buffered_buffer, regular_buffer, buffered_bytes_read) == 0);

    printf("test_read passed\n");
}

void test_append() {
    // Initial data
    const char *initial_data = "Initial data.";
    size_t initial_data_len = strlen(initial_data);

    // Append data
    const char *append_data = " Appended data.";
    size_t append_data_len = strlen(append_data);

    // Write initial data
    int fd = open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(fd != -1);
    write(fd, initial_data, initial_data_len);
    close(fd);

    // Append using regular functions
    fd = open(TEST_FILE, O_WRONLY | O_APPEND);
    assert(fd != -1);
    write(fd, append_data, append_data_len);
    close(fd);

    // Append using buffered functions
    buffered_file_t *bf = buffered_open(TEST_FILE, O_WRONLY | O_APPEND);
    assert(bf != NULL);
    buffered_write(bf, append_data, append_data_len);
    buffered_close(bf);

    // Read and compare
    FILE *f = fopen(TEST_FILE, "r");
    assert(f != NULL);
    char buffer[BUFFER_SIZE];
    size_t bytes_read = fread(buffer, 1, BUFFER_SIZE, f);
    fclose(f);

    // Assert the contents are the same
    const char *expected_data = "Initial data. Appended data. Appended data.";
    assert(bytes_read == strlen(expected_data));
    assert(memcmp(buffer, expected_data, bytes_read) == 0);

    printf("test_append passed\n");
}

void test_pre_append() {
    // Initial data to write
    const char *initial_data = "World!";
    size_t initial_data_len = strlen(initial_data);

    // Prepend data
    const char *prepend_data = "Hello ";
    size_t prepend_data_len = strlen(prepend_data);

    // Step 1: Write initial data using regular functions
    int fd = open(TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(fd != -1);
    write(fd, initial_data, initial_data_len);
    close(fd);

    // Step 2: Prepend data using buffered functions with O_PREAPPEND flag
    buffered_file_t *bf = buffered_open(TEST_FILE, O_RDWR | O_PREAPPEND);
    assert(bf != NULL);
    ssize_t buffered_bytes_written = buffered_write(bf, prepend_data, prepend_data_len);
    assert(buffered_bytes_written == prepend_data_len);
    buffered_close(bf);

    // Step 3: Read the file and check the contents
    FILE *f = fopen(TEST_FILE, "r");
    assert(f != NULL);
    char buffer[BUFFER_SIZE];
    size_t bytes_read = fread(buffer, 1, BUFFER_SIZE, f);
    fclose(f);

    // Expected data after prepend
    const char *expected_data = "Hello World!";
    size_t expected_data_len = strlen(expected_data);

    // Assert the contents are as expected
    assert(bytes_read == expected_data_len);
    assert(memcmp(buffer, expected_data, expected_data_len) == 0);

    printf("test_pre_append passed\n");
}

void test_pre_append_multiple() {
    // Initial data to write
    const char *initial_data = "data3";
    size_t initial_data_len = strlen(initial_data);

    // Prepend data
    const char *prepend_data1 = "data2 ";
    size_t prepend_data1_len = strlen(prepend_data1);
    const char *prepend_data2 = "data1 ";
    size_t prepend_data2_len = strlen(prepend_data2);

    // Step 1: Write initial data using regular functions
    int fd = open(TEST_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
    assert(fd != -1);
    write(fd, initial_data, initial_data_len);
    close(fd);

    // Step 2: Prepend data1 using buffered functions with O_PREAPPEND flag
    buffered_file_t *bf = buffered_open(TEST_FILE, O_RDWR | O_PREAPPEND);
    assert(bf != NULL);
    ssize_t buffered_bytes_written1 = buffered_write(bf, prepend_data1, prepend_data1_len);
    assert(buffered_bytes_written1 == prepend_data1_len);
    buffered_close(bf);

    // Step 3: Prepend data2 using buffered functions with O_PREAPPEND flag
    bf = buffered_open(TEST_FILE, O_RDWR | O_PREAPPEND);
    assert(bf != NULL);
    ssize_t buffered_bytes_written2 = buffered_write(bf, prepend_data2, prepend_data2_len);
    assert(buffered_bytes_written2 == prepend_data2_len);
    buffered_close(bf);

    // Step 4: Read the file and check the contents
    FILE *f = fopen(TEST_FILE, "r");
    assert(f != NULL);
    char buffer[BUFFER_SIZE];
    size_t bytes_read = fread(buffer, 1, BUFFER_SIZE, f);
    fclose(f);

    // Expected data after multiple prepends
    const char *expected_data = "data1 data2 data3";
    size_t expected_data_len = strlen(expected_data);

    // Assert the contents are as expected
    assert(bytes_read == expected_data_len);
    assert(memcmp(buffer, expected_data, expected_data_len) == 0);

    printf("test_pre_append_multiple passed\n");
}

int main() {
    chdir("../");
/*
    // Write initial data
    int fd = open(TEST_FILE, O_RDWR, 0644);
    assert(fd != -1);
    // lseek(fd, 0, SEEK_SET); // set to start of file
    char buf[] = "abc";
    size_t n = write(fd, buf, 3);
    printf("%zu", n);
    lseek(fd, 1, SEEK_SET); // set to start of file
    n = read(fd, buf, 3);
    close(fd);
*/
    test_open_and_write();
    test_read();
    test_append();
    test_pre_append();
    test_pre_append_multiple();

    printf("All tests passed successfully.\n");
    return 0;
}

