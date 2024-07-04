#!/bin/bash

# Directory names
SRC_DIR="source_directory"
DEST_DIR="destination_directory"
TEXT_TO_ADD="Hello, this is a test text."

# Clean up any existing directories
rm -rf "$SRC_DIR" "$DEST_DIR"

# Set up source directory structure
mkdir -p "$SRC_DIR/subdir1"
mkdir -p "$SRC_DIR/subdir2"
touch "$SRC_DIR/file1.txt"
chmod 777 "$SRC_DIR/file1.txt"  # Set specific permissions for file1.txt
touch "$SRC_DIR/file2.txt"
touch "$SRC_DIR/subdir1/file3.txt"
touch "$SRC_DIR/subdir1/file4.txt"
touch "$SRC_DIR/subdir2/file5.txt"
ln -s ../file1.txt "$SRC_DIR/subdir2/link_to_file1"

# Add text to a source file
echo "$TEXT_TO_ADD" > "$SRC_DIR/file1.txt"

# Compile the library
gcc -c copytree.c -o copytree.o
ar rcs libcopytree.a copytree.o

# Compile the main program
gcc part4.c -L. -lcopytree -o part4_program

# Run the program to copy the directory structure
./part4_program "$SRC_DIR" "$DEST_DIR"

# Clear destination before running with symbolic links and permissions
rm -rf "$DEST_DIR"

# Run the program with symbolic links and permissions
./part4_program -l -p "$SRC_DIR" "$DEST_DIR"

# Verify the results
if command -v tree &> /dev/null; then
    tree "$DEST_DIR"
else
    echo "Please install 'tree' to visualize the directory structure"
    # Fallback to manual checking if 'tree' is not available
    find "$DEST_DIR" -type l -exec ls -l {} +
fi

# Verify permissions
verify_permissions() {
    local src_file=$1
    local dest_file=$2

    local src_permissions=$(stat -c "%a" "$src_file")
    local dest_permissions=$(stat -c "%a" "$dest_file")

    if [ "$src_permissions" == "$dest_permissions" ]; then
        echo "Permissions are the same for $src_file and $dest_file: $src_permissions"
    else
        echo "Permissions differ for $src_file ($src_permissions) and $dest_file ($dest_permissions)"
    fi
}

# Verify permissions recursively
verify_permissions_recursive() {
    local src_dir=$1
    local dest_dir=$2

    find "$src_dir" -type f | while read -r src_file; do
        local relative_path="${src_file#$src_dir/}"
        local dest_file="$dest_dir/$relative_path"
        verify_permissions "$src_file" "$dest_file"
    done
}

verify_permissions_recursive "$SRC_DIR" "$DEST_DIR"

# Check symbolic links
find "$DEST_DIR" -type l | while read -r link; do
    target=$(readlink "$link")
    echo "Symbolic link: $link -> $target"
done

# Verify file content
verify_content() {
    local src_file=$1
    local dest_file=$2

    local src_content=$(cat "$src_file")
    local dest_content=$(cat "$dest_file")

    if [ "$src_content" == "$dest_content" ]; then
        echo "Content is the same for $src_file and $dest_file"
    else
        echo "Content differs for $src_file and $dest_file"
    fi
}

# Verify content recursively
verify_content_recursive() {
    local src_dir=$1
    local dest_dir=$2

    find "$src_dir" -type f | while read -r src_file; do
        local relative_path="${src_file#$src_dir/}"
        local dest_file="$dest_dir/$relative_path"
        verify_content "$src_file" "$dest_file"
    done
}

verify_content_recursive "$SRC_DIR" "$DEST_DIR"
