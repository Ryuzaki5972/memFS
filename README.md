# Memory File System (MemFS)

## Overview

Memory File System (MemFS) is a high-performance, in-memory file system implementation in C++ that simulates traditional file system operations without writing to disk. It provides a familiar command-line interface similar to Unix/Linux shell commands while maintaining all data structures in memory for rapid access and manipulation.

## Features

- **In-Memory Storage**: All files and directories are stored in RAM for ultra-fast operations
- **Thread-Safe Operations**: Concurrent file access through mutex synchronization
- **Hierarchical Directory Structure**: Complete support for nested directory structures
- **Familiar Command Interface**: Unix-like commands (ls, cd, mkdir, etc.)
- **Parallel File Operations**: Multi-threaded batch operations for file creation, deletion, and writing
- **Persistence Options**: Save and restore the entire file system to/from disk
- **Detailed Metadata**: Track file/directory creation and modification times
- **Path Normalization**: Robust handling of relative and absolute paths, including "." and ".." components

## Technical Specifications

- **Implementation Language**: C++
- **Data Structure**: Hash map-based for O(1) access to files and directories
- **Concurrency Model**: Thread-safe operations via mutex locks
- **Memory Management**: Dynamic allocation of string data for file contents
- **Size Tracking**: Automatic size calculation and tracking for all files

## Installation

### Prerequisites

- C++ compiler with C++17 support (GCC 7+, Clang 5+, MSVC 2017+)
- Standard Template Library (STL)
- CMake 3.10+ (optional, for build automation)

### Building from Source

1. Clone the repository or download the source code:

```bash
git clone https://github.com/yourusername/memfs.git
cd memfs
```

2. Compile the source code:

```bash
# Using g++
g++ -std=c++17 -pthread memFS.cpp -o memfs

# Using clang
clang++ -std=c++17 -pthread memFS.cpp -o memfs

# Using MSVC
cl /EHsc /std:c++17 memFS.cpp /Fe:memfs.exe
```

3. (Optional) Using CMake:

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

### Running the Program

```bash
./memfs
```

### Available Commands

| Command | Description | Example |
|---------|-------------|---------|
| `ls` | List files in current directory | `ls` |
| `ls -l` | List files with detailed information | `ls -l` |
| `ls <path>` | List files in specified directory | `ls /documents` |
| `cd <path>` | Change directory | `cd /documents` |
| `pwd` | Print working directory | `pwd` |
| `create <filename>` | Create empty file | `create myfile.txt` |
| `create -n <count> <files>` | Create multiple files | `create -n 3 file1 file2 file3` |
| `mkdir <dirname>` | Create directory | `mkdir documents` |
| `write <file> <content>` | Write content to file | `write myfile.txt "Hello World"` |
| `write -n <count> <file1> <content1> ...` | Write to multiple files | `write -n 2 file1 "Hello" file2 "World"` |
| `read <file>` | Read content from file | `read myfile.txt` |
| `delete <file>` | Delete file | `delete myfile.txt` |
| `delete -n <count> <files>` | Delete multiple files | `delete -n 2 file1 file2` |
| `rmdir <dir>` | Remove empty directory | `rmdir documents` |
| `rmdir -r <dir>` | Remove directory and contents | `rmdir -r documents` |
| `mv <src> <dest>` | Move/rename file or directory | `mv file1 file2` |
| `cp <src> <dest>` | Copy file or directory | `cp file1 file2` |
| `search <pattern>` | Search for files matching pattern | `search .txt` |
| `info <path>` | Display detailed information | `info myfile.txt` |
| `save <file>` | Save memory file system to disk | `save backup.dat` |
| `load <file>` | Load memory file system from disk | `load backup.dat` |
| `stats` | Display system statistics | `stats` |
| `help` | Display help information | `help` |
| `exit` | Exit the program | `exit` |

## Example Usage Scenarios

### Basic File Operations

```
/> mkdir documents
Directory created successfully: /documents

/> cd documents
Changed directory to: /documents

/documents> create myfile.txt
File created successfully: /documents/myfile.txt

/documents> write myfile.txt "This is a test file created in the memory file system."
Successfully written to /documents/myfile.txt

/documents> read myfile.txt
Content of /documents/myfile.txt: This is a test file created in the memory file system.

/documents> ls -l
Type    Size    Created     Last Modified    Name
FILE    56      20/05/2025  20/05/2025       myfile.txt
```

### Batch Operations

```
/> create -n 3 file1.txt file2.txt file3.txt
File created successfully: /file1.txt
File created successfully: /file2.txt
File created successfully: /file3.txt

/> write -n 3 file1.txt "Content 1" file2.txt "Content 2" file3.txt "Content 3"
Successfully written to /file1.txt
Successfully written to /file2.txt
Successfully written to /file3.txt

/> ls -l
Type    Size    Created     Last Modified    Name
FILE    9       20/05/2025  20/05/2025       file1.txt
FILE    9       20/05/2025  20/05/2025       file2.txt
FILE    9       20/05/2025  20/05/2025       file3.txt
DIR     0       20/05/2025  20/05/2025       documents
```

### Directory Management

```
/> mkdir -p projects/cpp/memfs
Directory created successfully: /projects
Directory created successfully: /projects/cpp
Directory created successfully: /projects/cpp/memfs

/> cd projects/cpp/memfs
Changed directory to: /projects/cpp/memfs

/projects/cpp/memfs> pwd
Current directory: /projects/cpp/memfs

/projects/cpp/memfs> cd ../../..
Changed directory to: /
```

### Saving and Loading the File System

```
/> save filesystem_backup.dat
File system saved to: filesystem_backup.dat

# Later, after restarting the program
/> load filesystem_backup.dat
File system loaded from: filesystem_backup.dat
```

## Technical Design

### Key Components

1. **Data Structures**
   - `EntryType` enum: Distinguishes between files and directories
   - `FSEntry` struct: Stores metadata and content for each file/directory
   - `unordered_map`: Main storage container mapping paths to entries

2. **Path Management**
   - Path normalization to handle relative paths, ".", and ".."
   - Automatic creation of parent directories when needed
   - Robust path parsing and validation

3. **Concurrency Control**
   - Mutex-based synchronization for thread safety
   - Thread pooling for batch operations
   - Fine-grained locking to minimize contention

4. **File System Operations**
   - Core operations: create, read, write, delete
   - Directory operations: mkdir, rmdir, cd
   - Advanced operations: move, copy, search

## Performance Considerations

- **Time Complexity**: Most operations have O(1) time complexity due to the hash map implementation
- **Space Complexity**: O(n) where n is the total size of all files and associated metadata
- **Memory Usage**: Keep file sizes reasonable as all content is stored in RAM
- **Concurrency**: The system can handle multiple threads accessing different files concurrently

## Limitations

- All data is stored in memory, so system RAM limits the total file system size
- No user permissions or access control functionality
- No journaling or transaction support for crash recovery
- Limited support for special file types (no symbolic links, device files, etc.)

## Future Enhancements

- Add user authentication and file permissions
- Implement file compression to reduce memory usage
- Add journaling for crash recovery
- Support for symbolic links
- Add network file sharing capabilities
- Implement memory-mapped file access for improved performance
