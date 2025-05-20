// Include necessary standard libraries
#include <iostream>     // For input/output operations
#include <string>       // For string manipulation
#include <unordered_map> // For hash map implementation
#include <vector>       // For dynamic arrays
#include <sstream>      // For string stream processing
#include <chrono>       // For time-related functions
#include <iomanip>      // For input/output manipulation
#include <thread>       // For multi-threading support
#include <mutex>        // For thread synchronization
#include <fstream>      // For file operations
#include <ctime>        // For C-style time functions
#include <algorithm>    // For standard algorithms
#include <filesystem>   // For path manipulation

/**
 * Enum representing the type of entry in the file system
 */
enum class EntryType {
    FILE,
    DIRECTORY
};

/**
 * Structure representing an entry in the memory file system
 */
struct FSEntry {
    std::string data;              // Content of the file (empty for directories)
    size_t sizeInBytes;            // Size of the file in bytes (0 for directories)
    std::string creationDate;      // Date when the entry was created
    std::string modificationDate;  // Date when the entry was last modified
    EntryType type;                // Type of entry (file or directory)
};

// Global variables
std::unordered_map<std::string, FSEntry> memoryFileSystem;  // Main data structure to store files and directories
std::string currentDirectory = "/";                         // Current working directory
std::mutex fileSystemMutex;                                 // Mutex for thread-safe operations

/**
 * Gets the current date as a formatted string
 * @return String representation of the current date in DD/MM/YYYY format
 */
std::string getCurrentDateString() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm* tm = std::localtime(&time);
    std::stringstream dateStream;
    dateStream << std::put_time(tm, "%d/%m/%Y");
    return dateStream.str();
}

/**
 * Splits a string into tokens based on a delimiter
 * @param input The string to tokenize
 * @param delimiter The character to use as delimiter (default: space)
 * @return Vector of tokens
 */
std::vector<std::string> tokenize(const std::string& input, char delimiter = ' ') {
    std::vector<std::string> tokens;
    size_t begin = 0, end = 0;
    
    // Find each token separated by the delimiter
    while ((end = input.find(delimiter, begin)) != std::string::npos) {
        if (end != begin) {
            tokens.push_back(input.substr(begin, end - begin));
        }
        begin = end + 1;
    }
    
    // Add the last token if it exists
    if (begin < input.size()) {
        tokens.push_back(input.substr(begin));
    }
    
    return tokens;
}

/**
 * Normalizes a path by handling relative paths and ensuring proper formatting
 * @param path The path to normalize
 * @return Normalized absolute path
 */
std::string normalizePath(const std::string& path) {
    std::string normalizedPath;
    
    // Handle absolute vs relative path
    if (path.empty() || path[0] != '/') {
        // Relative path - prepend current directory
        normalizedPath = currentDirectory;
        if (normalizedPath != "/" && path[0] != '/') {
            normalizedPath += "/";
        }
        normalizedPath += path;
    } else {
        // Absolute path
        normalizedPath = path;
    }
    
    // Process . and .. components
    std::vector<std::string> components;
    std::istringstream pathStream(normalizedPath);
    std::string component;
    
    while (std::getline(pathStream, component, '/')) {
        if (component == ".") {
            // Ignore "." components
            continue;
        } else if (component == "..") {
            // Go up one directory
            if (!components.empty()) {
                components.pop_back();
            }
        } else if (!component.empty()) {
            // Add valid component
            components.push_back(component);
        }
    }
    
    // Reconstruct path
    normalizedPath = "/";
    for (const auto& comp : components) {
        normalizedPath += comp + "/";
    }
    
    // Remove trailing slash except for root
    if (normalizedPath.length() > 1 && normalizedPath.back() == '/') {
        normalizedPath.pop_back();
    }
    
    return normalizedPath;
}

/**
 * Extracts the directory part from a path
 * @param path The full path
 * @return The directory part of the path
 */
std::string getDirectoryFromPath(const std::string& path) {
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos) {
        return "/";
    }
    return path.substr(0, lastSlash);
}

/**
 * Extracts the filename part from a path
 * @param path The full path
 * @return The filename part of the path
 */
std::string getFilenameFromPath(const std::string& path) {
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos) {
        return path;
    }
    return path.substr(lastSlash + 1);
}

/**
 * Checks if a directory exists
 * @param path The directory path to check
 * @return True if the directory exists, false otherwise
 */
bool directoryExists(const std::string& path) {
    auto dirIter = memoryFileSystem.find(path);
    return dirIter != memoryFileSystem.end() && dirIter->second.type == EntryType::DIRECTORY;
}

/**
 * Checks if a file exists
 * @param path The file path to check
 * @return True if the file exists, false otherwise
 */
bool fileExists(const std::string& path) {
    auto fileIter = memoryFileSystem.find(path);
    return fileIter != memoryFileSystem.end() && fileIter->second.type == EntryType::FILE;
}

/**
 * Ensures that all parent directories exist for a given path
 * @param path The path to check
 * @return True if all parent directories exist or were created, false otherwise
 */
bool ensureParentDirectoriesExist(const std::string& path) {
    std::string dirPath = getDirectoryFromPath(path);
    
    // If the directory is root, it always exists
    if (dirPath == "/") {
        return true;
    }
    
    // Check if the parent directory exists, if not create it recursively
    if (!directoryExists(dirPath)) {
        // Create parent directories recursively
        if (!ensureParentDirectoriesExist(dirPath)) {
            return false;
        }
        
        // Create the immediate parent directory
        FSEntry dirEntry;
        dirEntry.data = "";
        dirEntry.sizeInBytes = 0;
        dirEntry.creationDate = getCurrentDateString();
        dirEntry.modificationDate = getCurrentDateString();
        dirEntry.type = EntryType::DIRECTORY;
        
        memoryFileSystem[dirPath] = dirEntry;
    }
    
    return true;
}

/**
 * Updates the content of an existing file
 * @param path The path of the file to update
 * @param content The new content to write
 * @return True if successful, false otherwise
 */
bool updateFileContent(const std::string& path, const std::string& content) {
    auto fileIterator = memoryFileSystem.find(path);
    if (fileIterator == memoryFileSystem.end() || fileIterator->second.type != EntryType::FILE) {
        std::cerr << "Error: " << path << " does not exist or is not a file\n";
        return false;
    }
    
    // Update file metadata
    fileIterator->second.data = content;
    fileIterator->second.sizeInBytes = content.size();
    fileIterator->second.modificationDate = getCurrentDateString();
    return true;
}

/**
 * Writes content to a file with thread safety
 * @param path The path of the file to write to
 * @param content The content to write
 * @return True if successful, false otherwise
 */
bool writeContentToFile(const std::string& path, const std::string& content) {
    std::lock_guard<std::mutex> lock(fileSystemMutex);  // Thread-safe lock
    
    std::string normalizedPath = normalizePath(path);
    
    // Ensure parent directories exist
    if (!ensureParentDirectoriesExist(normalizedPath)) {
        std::cerr << "Error: Failed to create parent directories for " << normalizedPath << "\n";
        return false;
    }
    
    bool success = false;
    if (fileExists(normalizedPath)) {
        success = updateFileContent(normalizedPath, content);
    } else {
        // Create a new file if it doesn't exist
        FSEntry newFile;
        newFile.data = content;
        newFile.sizeInBytes = content.size();
        newFile.creationDate = getCurrentDateString();
        newFile.modificationDate = getCurrentDateString();
        newFile.type = EntryType::FILE;
        
        memoryFileSystem[normalizedPath] = newFile;
        success = true;
    }
    
    if (success) {
        std::cout << "Successfully written to " << normalizedPath << "\n";
    }
    return success;
}

/**
 * Lists all entries in a directory
 * @param path The directory path to list
 * @param detailed Whether to show detailed information
 */
void listDirectory(const std::string& path, bool detailed) {
    std::lock_guard<std::mutex> lock(fileSystemMutex);
    
    std::string normalizedPath = normalizePath(path);
    
    // Check if the directory exists
    if (!directoryExists(normalizedPath)) {
        std::cerr << "Error: Directory does not exist: " << normalizedPath << "\n";
        return;
    }
    
    // Collect all entries in the directory
    std::vector<std::pair<std::string, FSEntry>> entries;
    std::string prefix = normalizedPath == "/" ? "/" : normalizedPath + "/";
    
    for (const auto& entry : memoryFileSystem) {
        // Skip entries not in this directory
        if (entry.first == normalizedPath) {
            continue; // Skip the directory itself
        }
        
        // Check if entry is directly under the directory
        if (entry.first.find(prefix) == 0) {
            std::string relativePath = entry.first.substr(prefix.length());
            if (relativePath.find('/') == std::string::npos) {
                entries.emplace_back(relativePath, entry.second);
            }
        }
    }
    
    // Display the entries
    if (entries.empty()) {
        std::cout << "No entries in directory: " << normalizedPath << "\n";
    } else {
        if (detailed) {
            // Detailed listing
            std::cout << "Type\tSize\tCreated\t\tLast Modified\tName\n";
            for (const auto& entry : entries) {
                std::string typeStr = entry.second.type == EntryType::FILE ? "FILE" : "DIR";
                std::cout << typeStr << "\t" 
                         << entry.second.sizeInBytes << "\t"
                         << entry.second.creationDate << "\t"
                         << entry.second.modificationDate << "\t"
                         << entry.first << "\n";
            }
        } else {
            // Simple listing
            for (const auto& entry : entries) {
                std::string suffix = entry.second.type == EntryType::DIRECTORY ? "/" : "";
                std::cout << entry.first << suffix << "\n";
            }
        }
    }
}

/**
 * Displays a detailed list of all files and directories in the current directory
 */
void displayFileListDetailed() {
    listDirectory(currentDirectory, true);
}

/**
 * Displays a simple list of all filenames in the current directory
 */
void displayFileList() {
    listDirectory(currentDirectory, false);
}

/**
 * Writes to multiple files in parallel using threads
 * @param paths Vector of file paths to write to
 * @param contents Vector of contents to write
 */
void writeToFileBatch(const std::vector<std::string>& paths,
                      const std::vector<std::string>& contents) {
    std::vector<std::thread> threads;
    
    // Create a thread for each file write operation
    for (size_t i = 0; i < paths.size(); ++i) {
        threads.emplace_back([&, i]() {
            writeContentToFile(paths[i], contents[i]);
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread: threads) {
        thread.join();
    }
}

/**
 * Parses and executes the write command
 * @param command The full command string to parse
 */
void parseWriteCommand(const std::string& command) {
    auto args = tokenize(command);
    if (args.size() < 3) {
        std::cerr << "Usage: write [-n <count>] <filename> <\"text to write\">\n";
        return;
    }
    
    size_t fileCount = 1;
    size_t startIndex = 1;
    
    // Check if multiple files are specified with -n flag
    if (args[1] == "-n") {
        fileCount = std::stoi(args[2]);
        startIndex = 3;
    }
    
    // Validate arguments for single file write
    if (fileCount == 1 && args.size() != startIndex + 2) {
        std::cerr << "Error: Invalid arguments for write command\n";
        return;
    }
    
    // Validate arguments for multiple file write
    if (fileCount > 1 && ((args.size() - startIndex) % 2 != 0 || (args.size() - startIndex) / 2 != fileCount)) {
        std::cerr << "Error: Invalid arguments for write command\n";
        return;
    }
    
    // Prepare filenames and contents for batch processing
    std::vector<std::string> paths;
    std::vector<std::string> contents;
    for (size_t i = startIndex; i < args.size(); i += 2) {
        paths.push_back(args[i]);
        contents.push_back(args[i + 1]);
    }
    
    writeToFileBatch(paths, contents);
}

/**
 * Reads and displays the content of a file
 * @param path The path of the file to read
 */
void readContentFromFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(fileSystemMutex);
    
    std::string normalizedPath = normalizePath(path);
    auto fileIterator = memoryFileSystem.find(normalizedPath);
    
    if (fileIterator == memoryFileSystem.end() || fileIterator->second.type != EntryType::FILE) {
        std::cerr << "Error: " << normalizedPath << " does not exist or is not a file\n";
    } else {
        std::cout << "Content of " << normalizedPath << ": " << fileIterator->second.data << "\n";
    }
}

/**
 * Adds a new file or directory to the system (internal implementation without mutex)
 * @param path The path of the file or directory to create
 * @param isDirectory Whether to create a directory or a file
 * @return True if successful, false otherwise
 */
bool addNewEntryInternal(const std::string& path, bool isDirectory) {
    std::string normalizedPath = normalizePath(path);
    
    // Check if the entry already exists
    if (memoryFileSystem.find(normalizedPath) != memoryFileSystem.end()) {
        std::cerr << "Error: Entry with the same path already exists: " << normalizedPath << "\n";
        return false;
    }
    
    // Ensure parent directories exist
    if (!ensureParentDirectoriesExist(normalizedPath)) {
        std::cerr << "Error: Failed to create parent directories for " << normalizedPath << "\n";
        return false;
    }
    
    // Create a new entry
    FSEntry newEntry;
    newEntry.data = "";
    newEntry.sizeInBytes = 0;
    newEntry.creationDate = getCurrentDateString();
    newEntry.modificationDate = getCurrentDateString();
    newEntry.type = isDirectory ? EntryType::DIRECTORY : EntryType::FILE;
    
    // Add the new entry to the memoryFileSystem
    memoryFileSystem[normalizedPath] = newEntry;
    
    std::string entryType = isDirectory ? "Directory" : "File";
    std::cout << entryType << " created successfully: " << normalizedPath << "\n";
    return true;
}

/**
 * Thread-safe wrapper for adding a new file
 * @param path The path of the file to create
 * @return True if successful, false otherwise
 */
bool addNewFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(fileSystemMutex);
    return addNewEntryInternal(path, false);
}

/**
 * Thread-safe wrapper for adding a new directory
 * @param path The path of the directory to create
 * @return True if successful, false otherwise
 */
bool addNewDirectory(const std::string& path) {
    std::lock_guard<std::mutex> lock(fileSystemMutex);
    return addNewEntryInternal(path, true);
}

/**
 * Creates multiple files in parallel using threads
 * @param paths Vector of file paths to create
 */
void createMultipleFiles(const std::vector<std::string>& paths) {
    std::vector<std::thread> threads;
    
    // Create a thread for each file creation
    for (const auto& path : paths) {
        threads.emplace_back([path]() {
            addNewFile(path);
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
}

/**
 * Parses and executes the create command
 * @param command The full command string to parse
 */
void parseCreateCommand(const std::string& command) {
    auto args = tokenize(command);
    if (args.size() < 2) {
        std::cerr << "Usage: create [-n <count>] <filename1> [<filename2> ...]\n";
        return;
    }
    
    size_t fileCount = 1;
    size_t startIndex = 1;
    
    // Check if multiple files are specified with -n flag
    if (args[1] == "-n") {
        fileCount = std::stoi(args[2]);
        startIndex = 3;
    }
    
    // Validate argument count
    if (args.size() - startIndex != fileCount) {
        std::cerr << "Error: Number of filenames doesn't match specified count\n";
        return;
    }
    
    std::vector<std::string> paths(args.begin() + startIndex, args.end());
    createMultipleFiles(paths);
}

/**
 * Creates a new directory
 * @param command The full command string to parse
 */
void parseMkdirCommand(const std::string& command) {
    auto args = tokenize(command);
    if (args.size() != 2) {
        std::cerr << "Usage: mkdir <directory_path>\n";
        return;
    }
    
    addNewDirectory(args[1]);
}

/**
 * Changes the current working directory
 * @param command The full command string to parse
 */
void parseCdCommand(const std::string& command) {
    auto args = tokenize(command);
    if (args.size() != 2) {
        std::cerr << "Usage: cd <directory_path>\n";
        return;
    }
    
    std::string targetDir = normalizePath(args[1]);
    
    std::lock_guard<std::mutex> lock(fileSystemMutex);
    
    // Handle special case for root directory
    if (targetDir == "/") {
        currentDirectory = "/";
        std::cout << "Changed directory to: " << currentDirectory << "\n";
        return;
    }
    
    // Check if the directory exists
    if (!directoryExists(targetDir)) {
        std::cerr << "Error: Directory does not exist: " << targetDir << "\n";
        return;
    }
    
    // Change current directory
    currentDirectory = targetDir;
    std::cout << "Changed directory to: " << currentDirectory << "\n";
}

/**
 * Prints the current working directory
 */
void printWorkingDirectory() {
    std::cout << "Current directory: " << currentDirectory << "\n";
}

/**
 * Removes a file or directory from the system (internal implementation without mutex)
 * @param path The path of the entry to remove
 * @param recursive Whether to remove directories recursively
 * @return True if successful, false otherwise
 */
bool removeEntryInternal(const std::string& path, bool recursive) {
    std::string normalizedPath = normalizePath(path);
    
    auto entryIterator = memoryFileSystem.find(normalizedPath);
    if (entryIterator == memoryFileSystem.end()) {
        std::cerr << "Error: " << normalizedPath << " does not exist\n";
        return false;
    }
    
    if (entryIterator->second.type == EntryType::DIRECTORY) {
        // Check for contents in the directory
        std::string prefix = normalizedPath == "/" ? "/" : normalizedPath + "/";
        bool hasContents = false;
        
        for (const auto& entry : memoryFileSystem) {
            if (entry.first != normalizedPath && entry.first.find(prefix) == 0) {
                hasContents = true;
                break;
            }
        }
        
        if (hasContents && !recursive) {
            std::cerr << "Error: Directory not empty, use 'rmdir -r' for recursive deletion\n";
            return false;
        }
        
        if (recursive) {
            // Remove all entries inside the directory
            std::vector<std::string> pathsToRemove;
            
            for (const auto& entry : memoryFileSystem) {
                if (entry.first != normalizedPath && entry.first.find(prefix) == 0) {
                    pathsToRemove.push_back(entry.first);
                }
            }
            
            for (const auto& p : pathsToRemove) {
                memoryFileSystem.erase(p);
            }
        }
    }
    
    memoryFileSystem.erase(entryIterator);
    
    std::string entryType = entryIterator->second.type == EntryType::DIRECTORY ? "Directory" : "File";
    std::cout << entryType << " deleted successfully: " << normalizedPath << "\n";
    return true;
}

/**
 * Thread-safe wrapper for removing a file
 * @param path The path of the file to remove
 * @return True if successful, false otherwise
 */
bool removeFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(fileSystemMutex);
    return removeEntryInternal(path, false);
}

/**
 * Thread-safe wrapper for removing a directory
 * @param path The path of the directory to remove
 * @param recursive Whether to remove recursively
 * @return True if successful, false otherwise
 */
bool removeDirectory(const std::string& path, bool recursive) {
    std::lock_guard<std::mutex> lock(fileSystemMutex);
    return removeEntryInternal(path, recursive);
}

/**
 * Deletes multiple files in parallel using threads
 * @param paths Vector of file paths to delete
 */
void deleteMultipleFiles(const std::vector<std::string>& paths) {
    std::vector<std::thread> threads;
    std::vector<std::string> missingFiles;
    
    // Create a thread for each file deletion
    for (const auto& path : paths) {
        threads.emplace_back([&missingFiles, path]() {
            if (!removeFile(path)) {
                fileSystemMutex.lock();
                missingFiles.push_back(path);
                fileSystemMutex.unlock();
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Report results
    if (missingFiles.empty()) {
        std::cout << "Files deleted successfully\n";
    } else {
        std::cout << "Some files were not found: ";
        for (const auto& file : missingFiles) {
            std::cout << file << " ";
        }
        std::cout << "\nRemaining files deleted successfully\n";
    }
}

/**
 * Parses and executes the delete command
 * @param command The full command string to parse
 */
void parseDeleteCommand(const std::string& command) {
    auto args = tokenize(command);
    if (args.size() < 2) {
        std::cerr << "Usage: delete [-n <count>] <filename1> [<filename2> ...]\n";
        return;
    }
    
    size_t fileCount = 1;
    size_t startIndex = 1;
    
    // Check if multiple files are specified with -n flag
    if (args[1] == "-n") {
        fileCount = std::stoi(args[2]);
        startIndex = 3;
    }
    
    // Validate argument count
    if (args.size() - startIndex != fileCount) {
        std::cerr << "Error: Number of filenames doesn't match specified count\n";
        return;
    }
    
    std::vector<std::string> paths(args.begin() + startIndex, args.end());
    deleteMultipleFiles(paths);
}

/**
 * Parses and executes the rmdir command
 * @param command The full command string to parse
 */
void parseRmdirCommand(const std::string& command) {
    auto args = tokenize(command);
    if (args.size() < 2 || args.size() > 3) {
        std::cerr << "Usage: rmdir [-r] <directory_path>\n";
        return;
    }
    
    bool recursive = false;
    std::string dirPath;
    
    if (args.size() == 3 && args[1] == "-r") {
        recursive = true;
        dirPath = args[2];
    } else {
        dirPath = args[1];
    }
    
    removeDirectory(dirPath, recursive);
}

/**
 * Moves or renames a file or directory
 * @param command The full command string to parse
 */
void parseMoveCommand(const std::string& command) {
    auto args = tokenize(command);
    if (args.size() != 3) {
        std::cerr << "Usage: mv <source_path> <destination_path>\n";
        return;
    }
    
    std::string sourcePath = normalizePath(args[1]);
    std::string destPath = normalizePath(args[2]);
    
    std::lock_guard<std::mutex> lock(fileSystemMutex);
    
    // Check if source exists
    auto sourceIter = memoryFileSystem.find(sourcePath);
    if (sourceIter == memoryFileSystem.end()) {
        std::cerr << "Error: Source does not exist: " << sourcePath << "\n";
        return;
    }
    
    // Check if destination exists
    if (memoryFileSystem.find(destPath) != memoryFileSystem.end()) {
        std::cerr << "Error: Destination already exists: " << destPath << "\n";
        return;
    }
    
    // If source is a directory, need to handle all contents
    if (sourceIter->second.type == EntryType::DIRECTORY) {
        // Create destination directory
        if (!addNewEntryInternal(destPath, true)) {
            return;
        }
        
        // Move all contents
        std::string sourcePrefix = sourcePath == "/" ? "/" : sourcePath + "/";
        std::string destPrefix = destPath == "/" ? "/" : destPath + "/";
        
        std::vector<std::pair<std::string, FSEntry>> entriesToMove;
        
        // Collect all entries to move
        for (const auto& entry : memoryFileSystem) {
            if (entry.first != sourcePath && entry.first.find(sourcePrefix) == 0) {
                std::string relativePath = entry.first.substr(sourcePrefix.length());
                std::string newPath = destPrefix + relativePath;
                entriesToMove.emplace_back(newPath, entry.second);
            }
        }
        
        // Add all entries to the destination
        for (const auto& entry : entriesToMove) {
            memoryFileSystem[entry.first] = entry.second;
        }
        
        // Remove source entries
        std::vector<std::string> pathsToRemove;
        for (const auto& entry : memoryFileSystem) {
            if (entry.first.find(sourcePrefix) == 0) {
                pathsToRemove.push_back(entry.first);
            }
        }
        
        for (const auto& path : pathsToRemove) {
            memoryFileSystem.erase(path);
        }
    } else {
        // For files, just move the entry
        memoryFileSystem[destPath] = sourceIter->second;
    }
    
    // Remove the source entry
    memoryFileSystem.erase(sourcePath);
    
    std::cout << "Successfully moved " << sourcePath << " to " << destPath << "\n";
}

/**
 * Copies a file or directory
 * @param command The full command string to parse
 */
void parseCopyCommand(const std::string& command) {
    auto args = tokenize(command);
    if (args.size() != 3) {
        std::cerr << "Usage: cp <source_path> <destination_path>\n";
        return;
    }
    
    std::string sourcePath = normalizePath(args[1]);
    std::string destPath = normalizePath(args[2]);
    
    std::lock_guard<std::mutex> lock(fileSystemMutex);
    
    // Check if source exists
    auto sourceIter = memoryFileSystem.find(sourcePath);
    if (sourceIter == memoryFileSystem.end()) {
        std::cerr << "Error: Source does not exist: " << sourcePath << "\n";
        return;
    }
    
    // Check if destination exists
    if (memoryFileSystem.find(destPath) != memoryFileSystem.end()) {
        std::cerr << "Error: Destination already exists: " << destPath << "\n";
        return;
    }
    
    // Create destination with current date
    FSEntry destEntry = sourceIter->second;
    destEntry.creationDate = getCurrentDateString();
    destEntry.modificationDate = getCurrentDateString();
    
    // If source is a directory, need to handle all contents
    if (sourceIter->second.type == EntryType::DIRECTORY) {
        // Create destination directory
        if (!addNewEntryInternal(destPath, true)) {
            return;
        }
        
        // Copy all contents
        std::string sourcePrefix = sourcePath == "/" ? "/" : sourcePath + "/";
        std::string destPrefix = destPath == "/" ? "/" : destPath + "/";
        
        // Collect and copy all entries
        for (const auto& entry : memoryFileSystem) {
            if (entry.first != sourcePath && entry.first.find(sourcePrefix) == 0) {
                std::string relativePath = entry.first.substr(sourcePrefix.length());
                std::string newPath = destPrefix + relativePath;
                
                // Create a new entry with updated timestamps
                FSEntry newEntry = entry.second;
                newEntry.creationDate = getCurrentDateString();
                newEntry.modificationDate = getCurrentDateString();
                
                memoryFileSystem[newPath] = newEntry;
            }
        }
    } else {
        // For files, just copy the entry
        memoryFileSystem[destPath] = destEntry;
    }
    
    std::cout << "Successfully copied " << sourcePath << " to " << destPath << "\n";
}

/**
 * Searches for files or directories matching a pattern
 * @param command The full command string to parse
 */
void parseSearchCommand(const std::string& command) {
    auto args = tokenize(command);
    if (args.size() != 2) {
        std::cerr << "Usage: search <pattern>\n";
        return;
    }
    
    std::string pattern = args[1];
    std::lock_guard<std::mutex> lock(fileSystemMutex);
    
    bool found = false;
    std::cout << "Search results for pattern: " << pattern << "\n";
    
    for (const auto& entry : memoryFileSystem) {
        std::string filename = getFilenameFromPath(entry.first);
        if (filename.find(pattern) != std::string::npos) {
            std::string typeStr = entry.second.type == EntryType::FILE ? "FILE" : "DIR";
            std::cout << typeStr << "\t" << entry.first << "\n";
            found = true;
        }
    }
    
    if (!found) {
        std::cout << "No matching entries found.\n";
    }
}

/**
 * Displays detailed information about a file or directory
 * @param command The full command string to parse
 */
void parseInfoCommand(const std::string& command) {
    auto args = tokenize(command);
    if (args.size() != 2) {
        std::cerr << "Usage: info <path>\n";
        return;
    }
    
    std::string normalizedPath = normalizePath(args[1]);
    std::lock_guard<std::mutex> lock(fileSystemMutex);
    
    auto entryIter = memoryFileSystem.find(normalizedPath);
    if (entryIter == memoryFileSystem.end()) {
        std::cerr << "Error: Entry does not exist: " << normalizedPath << "\n";
        return;
    }
    
    std::cout << "Information for: " << normalizedPath << "\n";
    std::cout << "Type: " << (entryIter->second.type == EntryType::FILE ? "File" : "Directory") << "\n";
    std::cout << "Size: " << entryIter->second.sizeInBytes << " bytes\n";
    std::cout << "Created: " << entryIter->second.creationDate << "\n";
    std::cout << "Modified: " << entryIter->second.modificationDate << "\n";
    
    if (entryIter->second.type == EntryType::DIRECTORY) {
        // Count number of direct children
        size_t childCount = 0;
        std::string prefix = normalizedPath == "/" ? "/" : normalizedPath + "/";
        
        for (const auto& entry : memoryFileSystem) {
            if (entry.first != normalizedPath && entry.first.find(prefix) == 0) {
                std::string relativePath = entry.first.substr(prefix.length());
                if (relativePath.find('/') == std::string::npos) {
                    childCount++;
                }
            }
        }
        
        std::cout << "Direct children: " << childCount << "\n";
    }
}

/**
 * Saves the memory file system to a physical file on disk
 * @param command The full command string to parse
 */
void parseSaveCommand(const std::string& command) {
    auto args = tokenize(command);
    if (args.size() != 2) {
        std::cerr << "Usage: save <filename>\n";
        return;
    }
    
    std::string filename = args[1];
    std::lock_guard<std::mutex> lock(fileSystemMutex);
    
    std::ofstream outFile(filename);
    if (!outFile) {
        std::cerr << "Error: Could not open file for writing: " << filename << "\n";
        return;
    }
    
    // Write header
    outFile << "# Memory File System Dump - " << getCurrentDateString() << "\n";
    outFile << "# Format: <type>|<path>|<size>|<created>|<modified>|<data>\n";
    
    // Write entries
    for (const auto& entry : memoryFileSystem) {
        std::string typeStr = entry.second.type == EntryType::FILE ? "FILE" : "DIR";
        
        outFile << typeStr << "|"
                << entry.first << "|"
                << entry.second.sizeInBytes << "|"
                << entry.second.creationDate << "|"
                << entry.second.modificationDate << "|";
        
        // Only write data for files
        if (entry.second.type == EntryType::FILE) {
            outFile << entry.second.data;
        }
        
        outFile << "\n";
    }
    
    outFile.close();
    std::cout << "File system saved to: " << filename << "\n";
}

/**
 * Loads the memory file system from a physical file on disk
 * @param command The full command string to parse
 */
void parseLoadCommand(const std::string& command) {
    auto args = tokenize(command);
    if (args.size() != 2) {
        std::cerr << "Usage: load <filename>\n";
        return;
    }
    
    std::string filename = args[1];
    std::lock_guard<std::mutex> lock(fileSystemMutex);
    
    std::ifstream inFile(filename);
    if (!inFile) {
        std::cerr << "Error: Could not open file for reading: " << filename << "\n";
        return;
    }
    
    // Clear existing file system
    memoryFileSystem.clear();
    
    std::string line;
    size_t lineNum = 0;
    
    // Skip header lines starting with #
    while (std::getline(inFile, line)) {
        lineNum++;
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Parse entry line
        std::stringstream ss(line);
        std::string typeStr, path, sizeStr, created, modified, data;
        
        // Split by | delimiter
        if (!std::getline(ss, typeStr, '|') ||
            !std::getline(ss, path, '|') ||
            !std::getline(ss, sizeStr, '|') ||
            !std::getline(ss, created, '|') ||
            !std::getline(ss, modified, '|')) {
            
            std::cerr << "Warning: Invalid format at line " << lineNum << ", skipping\n";
            continue;
        }
        
        // Get remaining part as data
        std::getline(ss, data);
        
        // Create entry
        FSEntry entry;
        entry.type = (typeStr == "FILE") ? EntryType::FILE : EntryType::DIRECTORY;
        entry.sizeInBytes = std::stoull(sizeStr);
        entry.creationDate = created;
        entry.modificationDate = modified;
        entry.data = data;
        
        memoryFileSystem[path] = entry;
    }
    
    inFile.close();
    std::cout << "File system loaded from: " << filename << "\n";
}

/**
 * Displays system statistics about the memory file system
 */
void displaySystemStats() {
    std::lock_guard<std::mutex> lock(fileSystemMutex);
    
    size_t totalFiles = 0;
    size_t totalDirs = 0;
    size_t totalSize = 0;
    
    for (const auto& entry : memoryFileSystem) {
        if (entry.second.type == EntryType::FILE) {
            totalFiles++;
            totalSize += entry.second.sizeInBytes;
        } else {
            totalDirs++;
        }
    }
    
    std::cout << "System Statistics:\n";
    std::cout << "Total Entries: " << memoryFileSystem.size() << "\n";
    std::cout << "Files: " << totalFiles << "\n";
    std::cout << "Directories: " << totalDirs << "\n";
    std::cout << "Total File Size: " << totalSize << " bytes\n";
}

/**
 * Displays help information about available commands
 */
void displayHelp() {
    std::cout << "\nMemory File System Commands:\n";
    std::cout << "---------------------------\n";
    std::cout << "ls                    - List files in current directory\n";
    std::cout << "ls -l                 - List files with details\n";
    std::cout << "ls <path>             - List files in specified directory\n";
    std::cout << "cd <path>             - Change directory\n";
    std::cout << "pwd                   - Print working directory\n";
    std::cout << "create <filename>     - Create empty file\n";
    std::cout << "create -n <n> <files> - Create multiple files\n";
    std::cout << "mkdir <dirname>       - Create directory\n";
    std::cout << "write <file> <content> - Write content to file\n";
    std::cout << "read <file>           - Read content from file\n";
    std::cout << "delete <file>         - Delete file\n";
    std::cout << "delete -n <n> <files> - Delete multiple files\n";
    std::cout << "rmdir <dir>           - Remove empty directory\n";
    std::cout << "rmdir -r <dir>        - Remove directory and contents\n";
    std::cout << "mv <src> <dest>       - Move/rename file or directory\n";
    std::cout << "cp <src> <dest>       - Copy file or directory\n";
    std::cout << "search <pattern>      - Search for files matching pattern\n";
    std::cout << "info <path>           - Display detailed information about a file or directory\n";
    std::cout << "save <file>           - Save memory file system to disk\n";
    std::cout << "load <file>           - Load memory file system from disk\n";
    std::cout << "stats                 - Display system statistics\n";
    std::cout << "help                  - Display this help information\n";
    std::cout << "exit                  - Exit the program\n";
}

/**
 * Initialize the memory file system with root directory
 */
void initializeFileSystem() {
    std::lock_guard<std::mutex> lock(fileSystemMutex);
    
    // Create root directory if it doesn't exist
    if (memoryFileSystem.find("/") == memoryFileSystem.end()) {
        FSEntry rootDir;
        rootDir.data = "";
        rootDir.sizeInBytes = 0;
        rootDir.creationDate = getCurrentDateString();
        rootDir.modificationDate = getCurrentDateString();
        rootDir.type = EntryType::DIRECTORY;
        
        memoryFileSystem["/"] = rootDir;
    }
}

/**
 * Main function to run the memory file system
 */
int main() {
    std::string command;
    
    // Initialize the file system with root directory
    initializeFileSystem();
    
    std::cout << "Memory File System v1.0\n";
    std::cout << "Type 'help' for available commands, 'exit' to quit.\n";
    
    while (true) {
        std::cout << currentDirectory << "> ";
        std::getline(std::cin, command);
        
        // Skip empty commands
        if (command.empty()) {
            continue;
        }
        
        // Extract the first word as the command name
        auto commandParts = tokenize(command);
        std::string commandName = commandParts[0];
        
        // Process the command
        if (commandName == "exit") {
            break;
        } else if (commandName == "help") {
            displayHelp();
        } else if (commandName == "ls") {
            if (commandParts.size() == 1) {
                displayFileList();
            } else if (commandParts.size() == 2) {
                if (commandParts[1] == "-l") {
                    displayFileListDetailed();
                } else {
                    listDirectory(commandParts[1], false);
                }
            } else if (commandParts.size() == 3 && commandParts[1] == "-l") {
                listDirectory(commandParts[2], true);
            } else {
                std::cerr << "Usage: ls [-l] [directory]\n";
            }
        } else if (commandName == "cd") {
            parseCdCommand(command);
        } else if (commandName == "pwd") {
            printWorkingDirectory();
        } else if (commandName == "create") {
            parseCreateCommand(command);
        } else if (commandName == "mkdir") {
            parseMkdirCommand(command);
        } else if (commandName == "write") {
            parseWriteCommand(command);
        } else if (commandName == "read") {
            if (commandParts.size() != 2) {
                std::cerr << "Usage: read <filename>\n";
            } else {
                readContentFromFile(commandParts[1]);
            }
        } else if (commandName == "delete") {
            parseDeleteCommand(command);
        } else if (commandName == "rmdir") {
            parseRmdirCommand(command);
        } else if (commandName == "mv") {
            parseMoveCommand(command);
        } else if (commandName == "cp") {
            parseCopyCommand(command);
        } else if (commandName == "search") {
            parseSearchCommand(command);
        } else if (commandName == "info") {
            parseInfoCommand(command);
        } else if (commandName == "save") {
            parseSaveCommand(command);
        } else if (commandName == "load") {
            parseLoadCommand(command);
        } else if (commandName == "stats") {
            displaySystemStats();
        } else {
            std::cerr << "Error: Unknown command: " << commandName << "\n";
            std::cerr << "Type 'help' for available commands.\n";
        }
    }
    
    std::cout << "Exiting Memory File System. Goodbye!\n";
    return 0;
}