# FileOps

**FileOps** is a collection of utilities designed for managing files and directories with advanced functionality. This toolkit provides features like buffered file operations, multi-process file writing, directory copying with optional symbolic link and permission preservation, and more.

---

## Features

1. **Buffered File Operations**
   - Perform efficient buffered file reading and writing with `buffered_open.c`.

2. **Multi-Process File Writing**
   - Handle concurrent file writes with controlled access using `part1.c` and `part2.c`.

3. **Directory Copying**
   - Copy entire directories with options to preserve symbolic links and file permissions using `part4.c` and `copytree.c`.

4. **Custom Utilities**
   - Extend file and directory management capabilities with reusable helper functions.

## Setup

### Prerequisites

- **GCC** (GNU Compiler Collection)
- **CMake** (for building the project)
- **Linux or Unix-based system**

### Installation

1. Clone the repository:
    ```bash
    git clone https://github.com/<username>/FileOps.git
    cd FileOps
    ```

2. Build the project using CMake:
    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```

3. The compiled executables will be available in the `build/` directory.

---

## Usage

### Buffered File Operations

The `buffered_open.c` module provides efficient buffered reading and writing for file operations. Include `buffered_open.h` in your project to use these features.

Example:
```c
buffered_file_t *file = buffered_open("example.txt", O_RDWR | O_CREAT, 0644);
buffered_write(file, "Hello, world!", 13);
buffered_flush(file);
buffered_close(file);


## Project Structure

```plaintext
FileOps/
├── CMakeLists.txt        # Build configuration for the project
├── buffered_open.c       # Buffered file operations implementation
├── buffered_open.h       # Header file for buffered file operations
├── copytree.c            # Implementation of directory copying utilities
├── copytree.h            # Header file for directory copying utilities
├── part1.c               # Multi-process file writing implementation
├── part2.c               # Concurrent file writing with lock implementation
├── part4.c               # Command-line utility for directory copying
└── README.md             # Documentation
