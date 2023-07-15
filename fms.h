#pragma once
#include<sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <stdexcept>
#include <bitset>
#include <algorithm>

struct File {
    std::string name;
    std::string content;
    std::string permissions;
    int fileSize;
    std::vector<int> blockIndices; // Track allocated disk blocks
};

struct Directory {
    std::unordered_map<std::string, File> files;
    std::vector<std::string> subdirectories;
};

class FileSystem {
private:
    static const int MAX_FILES = 1000;  // Maximum number of files in the file system
    static const int MAX_DIRS = 100;    // Maximum number of directories in the file system
    static const int MAX_CAPACITY = 10000;  // Maximum storage capacity of the file system

    std::unordered_map<std::string, Directory> directoryStructure;
    std::unordered_map<std::string, std::bitset<1024>> fileAllocationMap;
    std::bitset<1024> diskBlockMap; // Bitmap to track disk block allocation
    std::string currentDirectory;  // Track the current directory

public:
    FileSystem() {
        loadFileSystem();
    }

    ~FileSystem() {
        saveFileSystem();
    }

    void runCLI() {
        std::cout << "Welcome to the CLI File System!\n";
        printHelp();

        std::string command;
        while (true) {
            std::cout << "\n";
            std::cout << currentDirectory << "> ";
            std::getline(std::cin, command);

            if (command.empty()) {
                continue;
            }

            std::vector<std::string> tokens = tokenizeCommand(command);
            std::string mainCommand = tokens[0];

            try {
                if (mainCommand == "cd") {
                    if (tokens.size() < 2) {
                        throw std::invalid_argument("Invalid command syntax! Usage: cd <directory>");
                    }
                    changeDirectory(tokens[1]);
                } else if (mainCommand == "createfile") {
                    if (tokens.size() < 4) {
                        throw std::invalid_argument("Invalid command syntax! Usage: createfile <name> <permissions> <size>");
                    }
                    std::string name = tokens[1];
                    std::string permissions = tokens[2];
                    int size = std::stoi(tokens[3]);
                    createFile(name, permissions, size);
                } else if (mainCommand == "writefile") {
                    if (tokens.size() < 3) {
                        throw std::invalid_argument("Invalid command syntax! Usage: writefile <name> <content>");
                    }
                    std::string name = tokens[1];
                    std::string content;
                    for (size_t i = 2; i < tokens.size(); ++i) {
                        content += tokens[i];
                        if (i < tokens.size() - 1) {
                            content += ' '; // Add a space between tokens
                        }
                    }
                    writeFile(name, content);
                } else if (mainCommand == "readfile") {
                    if (tokens.size() < 2) {
                        throw std::invalid_argument("Invalid command syntax! Usage: readfile <name>");
                    }
                    std::string name = tokens[1];
                    readFile(name);
                } else if (mainCommand == "deletefile") {
                    if (tokens.size() < 2) {
                        throw std::invalid_argument("Invalid command syntax! Usage: deletefile <name>");
                    }
                    std::string name = tokens[1];
                    deleteFile(name);
                } else if (mainCommand == "ls") {
                    listDirectory();
                } else if (mainCommand == "mkdir") {
                    if (tokens.size() < 2) {
                        throw std::invalid_argument("Invalid command syntax! Usage: mkdir <name>");
                    }
                    std::string name = tokens[1];
                    createDirectory(name);
                } else if (mainCommand == "mv") {
                    if (tokens.size() < 3) {
                        throw std::invalid_argument("Invalid command syntax! Usage: mv <source> <destination>");
                    }
                    std::string source = tokens[1];
                    std::string destination = tokens[2];
                    moveDirectory(source, destination);
                } else if (mainCommand == "rename") {
                    if (tokens.size() < 3) {
                        throw std::invalid_argument("Invalid command syntax! Usage: rename <old name> <new name>");
                    }
                    std::string oldName = tokens[1];
                    std::string newName = tokens[2];
                    renameEntry(oldName, newName);
                } else if (mainCommand == "appendfile") {
                    if (tokens.size() < 3) {
                        throw std::invalid_argument("Invalid command syntax! Usage: appendfile <name> <content>");
                    }
                    std::string name = tokens[1];
                    std::string content = tokens[2];
                    appendFile(name, content);
                } else if (mainCommand == "help") {
                    printHelp();
                } else if (mainCommand == "exit") {
                    break;
                } else {
                    throw std::invalid_argument("Invalid command! Type 'help' to see the available commands.");
                }
            } catch (const std::exception& ex) {
                std::cerr << "Error: " << ex.what() << "\n";
            }
        }
    }

private:
    void createFile(const std::string& name, const std::string& permissions, int size) {
        validateFileName(name);
        validateFileSize(size);

        Directory& currentDir = directoryStructure[currentDirectory];

        // Check if the file already exists in the current directory
        if (currentDir.files.find(name) != currentDir.files.end()) {
            throw std::invalid_argument("File already exists in the current directory!");
        }

        // Check if the maximum number of files has been reached
        if (currentDir.files.size() >= MAX_FILES) {
            throw std::runtime_error("Maximum number of files in the directory reached!");
        }

        File newFile;
        newFile.name = name;
        newFile.permissions = permissions;
        newFile.fileSize = size;

        allocateFileBlocks(newFile, size);
        currentDir.files[name] = newFile;

        saveFileSystem();
        std::cout << "File created successfully.\n";
    }

    void writeFile(const std::string& name, const std::string& content) {
        validateEntryExistence(name);

        File& file = directoryStructure[currentDirectory].files[name];
        file.content = content;

        saveFileSystem();
        std::cout << "File written successfully.\n";
    }

    void readFile(const std::string& name) {
        validateEntryExistence(name);

        const File& file = directoryStructure[currentDirectory].files[name];
        std::cout << "File content:\n";
        std::cout << file.content << std::endl;
    }

    void deleteFile(const std::string& name) {
        validateEntryExistence(name);

        File& file = directoryStructure[currentDirectory].files[name];
        deallocateFileBlocks(file);

        directoryStructure[currentDirectory].files.erase(name);

        saveFileSystem();
        std::cout << "File deleted successfully.\n";
    }

    void listDirectory() {
        const Directory& currentDir = directoryStructure[currentDirectory];
        std::cout << "Directory: " << currentDirectory << std::endl;

        for (const auto& filePair : currentDir.files) {
            const File& file = filePair.second;
            std::cout << "- " << file.name << " [" << file.permissions << "]" << std::endl;
        }

        for (const std::string& subdir : currentDir.subdirectories) {
            std::cout << "> " << subdir << std::endl;
        }
    }

    void createDirectory(const std::string& name) {
        validateDirectoryName(name);
        validateEntrynonExistence(name);

        if (directoryStructure.size() >= MAX_DIRS) {
            throw std::runtime_error("File system reached maximum directory limit!");
        }

        Directory newDir;
        directoryStructure[name] = newDir;

        directoryStructure[currentDirectory].subdirectories.push_back(name);

        saveFileSystem();
        std::cout << "Directory created successfully.\n";
    }

    void moveDirectory(const std::string& source, const std::string& destination) {
        validateDirectoryExistence(source);
        validateEntrynonExistence(destination);

        if (source == destination) {
            throw std::invalid_argument("Source and destination directories are the same!");
        }

        if (isSubdirectory(destination, source)) {
            throw std::invalid_argument("Cannot move directory inside its subdirectory!");
        }

        Directory& sourceDir = directoryStructure[source];
        Directory& destDir = directoryStructure[destination];

        destDir.subdirectories.push_back(source);
        sourceDir.subdirectories.erase(
            std::remove(sourceDir.subdirectories.begin(), sourceDir.subdirectories.end(), source),
            sourceDir.subdirectories.end()
        );

        saveFileSystem();
        std::cout << "Directory moved successfully.\n";
    }

void renameEntry(const std::string& oldName, const std::string& newName) {
    validateEntryExistence(oldName);
    validateEntryExistence(newName);

    if (newName.empty()) {
        throw std::invalid_argument("New name cannot be empty!");
    }

    if (newName.find('/') != std::string::npos) {
        throw std::invalid_argument("New name cannot contain '/' character!");
    }

    Directory& currentDir = directoryStructure[currentDirectory];

    if (currentDir.files.find(oldName) != currentDir.files.end()) {
        File& file = currentDir.files[oldName];
        file.name = newName;
    } else {
        for (std::string& subdir : currentDir.subdirectories) {
            if (subdir == oldName) {
                subdir = newName;
                break;
            }
        }
    }

    saveFileSystem();
    std::cout << "Entry renamed successfully.\n";
}

    void appendFile(const std::string& name, const std::string& content) {
        validateEntryExistence(name);

        File& file = directoryStructure[currentDirectory].files[name];
        int newSize = file.content.size() + content.size();

        if (newSize > file.fileSize) {
            throw std::runtime_error("File size exceeded!");
        }

        file.content += content;

        saveFileSystem();
        std::cout << "Content appended to file successfully.\n";
    }

    void allocateFileBlocks(File& file, int size) {
        int requiredBlocks = (size + 1023) / 1024;

        if (requiredBlocks > MAX_CAPACITY - diskBlockMap.count()) {
            throw std::runtime_error("Insufficient storage space to allocate file blocks!");
        }

        std::vector<int> freeBlocks = findFreeBlocks(requiredBlocks);

        if (freeBlocks.size() != requiredBlocks) {
            throw std::invalid_argument("File size exceeds available space!");
        }

        for (int block : freeBlocks) {
            file.blockIndices.push_back(block);
            diskBlockMap.set(block);
        }
    }

    void deallocateFileBlocks(const File& file) {
        for (int block : file.blockIndices) {
            diskBlockMap.reset(block);
        }
    }

    std::vector<int> findFreeBlocks(int numBlocks) {
        std::vector<int> freeBlocks;
        int count = 0;

        for (int i = 0; i < diskBlockMap.size() && count < numBlocks; ++i) {
            if (!diskBlockMap.test(i)) {
                freeBlocks.push_back(i);
                ++count;
            }
        }

        return freeBlocks;
    }

    void saveFileSystem() {
        std::ofstream file("filesystem.dat", std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to save the file system to disk!");
        }

        // Save directory structure size
        std::size_t directoryStructureSize = directoryStructure.size();
        file.write(reinterpret_cast<const char*>(&directoryStructureSize), sizeof(directoryStructureSize));

        // Save current directory
        std::size_t currentDirectorySize = currentDirectory.size();
        file.write(reinterpret_cast<const char*>(&currentDirectorySize), sizeof(currentDirectorySize));
        file.write(currentDirectory.data(), currentDirectorySize);

        // Save directory structure entries
        for (const auto& entry : directoryStructure) {
            const std::string& directoryName = entry.first;
            const Directory& directory = entry.second;

            // Save directory name size and name
            std::size_t directoryNameSize = directoryName.size();
            file.write(reinterpret_cast<const char*>(&directoryNameSize), sizeof(directoryNameSize));
            file.write(directoryName.data(), directoryNameSize);

            // Save file entries size
            std::size_t filesSize = directory.files.size();
            file.write(reinterpret_cast<const char*>(&filesSize), sizeof(filesSize));

            // Save file entries
            for (const auto& fileEntry : directory.files) {
                const std::string& fileName = fileEntry.first;
                const File& file_ = fileEntry.second;

                // Save file name size and name
                std::size_t fileNameSize = fileName.size();
                file.write(reinterpret_cast<const char*>(&fileNameSize), sizeof(fileNameSize));
                file.write(fileName.data(), fileNameSize);

                // Save file content size and content
                std::size_t contentSize = file_.content.size();
                file.write(reinterpret_cast<const char*>(&contentSize), sizeof(contentSize));
                file.write(file_.content.data(), contentSize);

                // Save file permissions size and permissions
                std::size_t permissionsSize = file_.permissions.size();
                file.write(reinterpret_cast<const char*>(&permissionsSize), sizeof(permissionsSize));
                file.write(file_.permissions.data(), permissionsSize);

                // Save file size
                file.write(reinterpret_cast<const char*>(&file_.fileSize), sizeof(file_.fileSize));
            }
        }

        file.close();
    }

    void loadFileSystem() {
        std::ifstream file("filesystem.dat", std::ios::binary);
        if (!file) {
            initializeFileSystem();
            return;
        }

        // Load directory structure size
        std::size_t directoryStructureSize;
        file.read(reinterpret_cast<char*>(&directoryStructureSize), sizeof(directoryStructureSize));

        // Load current directory
        std::size_t currentDirectorySize;
        file.read(reinterpret_cast<char*>(&currentDirectorySize), sizeof(currentDirectorySize));
        currentDirectory.resize(currentDirectorySize);
        file.read(&currentDirectory[0], currentDirectorySize);

        // Load directory structure entries
        for (std::size_t i = 0; i < directoryStructureSize; ++i) {
            // Load directory name size and name
            std::size_t directoryNameSize;
            file.read(reinterpret_cast<char*>(&directoryNameSize), sizeof(directoryNameSize));
            std::string directoryName(directoryNameSize, '\0');
            file.read(&directoryName[0], directoryNameSize);

            // Load file entries size
            std::size_t filesSize;
            file.read(reinterpret_cast<char*>(&filesSize), sizeof(filesSize));

            // Load file entries
            Directory directory;
            for (std::size_t j = 0; j < filesSize; ++j) {
                // Load file name size and name
                std::size_t fileNameSize;
                file.read(reinterpret_cast<char*>(&fileNameSize), sizeof(fileNameSize));
                std::string fileName(fileNameSize, '\0');
                file.read(&fileName[0], fileNameSize);

                // Load file content size and content
                std::size_t contentSize;
                file.read(reinterpret_cast<char*>(&contentSize), sizeof(contentSize));
                std::string content(contentSize, '\0');
                file.read(&content[0], contentSize);

                // Load file permissions size and permissions
                std::size_t permissionsSize;
                file.read(reinterpret_cast<char*>(&permissionsSize), sizeof(permissionsSize));
                std::string permissions(permissionsSize, '\0');
                file.read(&permissions[0], permissionsSize);

                // Load file size
                int fileSize;
                file.read(reinterpret_cast<char*>(&fileSize), sizeof(fileSize));

                // Create file entry and add it to the directory
                File fileEntry;
                fileEntry.name = fileName;
                fileEntry.content = content;
                fileEntry.permissions = permissions;
                fileEntry.fileSize = fileSize;
                directory.files[fileName] = fileEntry;
            }

            // Add directory to the directory structure
            directoryStructure[directoryName] = directory;
        }

        file.close();
    }

    void initializeFileSystem() {
        directoryStructure.clear();
        fileAllocationMap.clear();
        diskBlockMap.reset();

        Directory root;
        directoryStructure["/"] = root;
        currentDirectory = "/";

        saveFileSystem();
    }

    void changeDirectory(const std::string& directory) {
        if (directoryStructure.find(directory) == directoryStructure.end()) {
            throw std::invalid_argument("Directory not found!");
        }

        currentDirectory = directory;
        std::cout << "Changed directory to: " << currentDirectory << std::endl;
    }

    void printHelp() {
        std::cout << "Available commands:\n";
        std::cout << "- cd <directory>: Change directory\n";
        std::cout << "- createfile <name> <permissions> <size>: Create a new file\n";
        std::cout << "- writefile <name> <content>: Write content to a file\n";
        std::cout << "- readfile <name>: Read content from a file\n";
        std::cout << "- deletefile <name>: Delete a file\n";
        std::cout << "- ls: List files and directories in the current directory\n";
        std::cout << "- mkdir <name>: Create a new directory\n";
        std::cout << "- mv <source> <destination>: Move a directory\n";
        std::cout << "- rename <old name> <new name>: Rename a file or directory\n";
        std::cout << "- appendfile <name> <content>: Append content to an existing file\n";
        std::cout << "- help: Display available commands\n";
        std::cout << "- exit: Exit the file system\n";
    }

    std::vector<std::string> tokenizeCommand(const std::string& command) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(command);
        while (std::getline(tokenStream, token, ' ')) {
            tokens.push_back(token);
        }
        return tokens;
    }

    void validateFileName(const std::string& name) {
        if (name.empty()) {
            throw std::invalid_argument("File name cannot be empty!");
        }

        if (name.find('/') != std::string::npos) {
            throw std::invalid_argument("File name cannot contain '/' character!");
        }
    }

    void validateFileSize(int size) {
        if (size <= 0) {
            throw std::invalid_argument("File size must be positive!");
        }
    }

    void validateDirectoryName(const std::string& name) {
        if (name.empty()) {
            throw std::invalid_argument("Directory name cannot be empty!");
        }

        if (name.find('/') != std::string::npos) {
            throw std::invalid_argument("Directory name cannot contain '/' character!");
        }
    }
    void validateEntrynonExistence(const std::string& name) {
        if (directoryStructure.find(currentDirectory) == directoryStructure.end()) {
            throw std::invalid_argument("Current directory does not exist!");
        }

        const Directory& currentDir = directoryStructure[currentDirectory];
        if (currentDir.files.find(name) != currentDir.files.end()) {
            throw std::invalid_argument("File or directory ");
        }
    }

    void validateEntryExistence(const std::string& name) {
        if (directoryStructure.find(currentDirectory) == directoryStructure.end()) {
            throw std::invalid_argument("Current directory does not exist!");
        }

        const Directory& currentDir = directoryStructure[currentDirectory];
        if (currentDir.files.find(name) == currentDir.files.end()) {
            throw std::invalid_argument("File or directory not found!");
        }
    }

    void validateDirectoryExistence(const std::string& name) {
        if (directoryStructure.find(name) == directoryStructure.end()) {
            throw std::invalid_argument("Directory not found!");
        }
    }

    bool isSubdirectory(const std::string& source, const std::string& destination) {
        for (const std::string& subdir : directoryStructure[destination].subdirectories) {
            if (subdir == source || isSubdirectory(source, subdir)) {
                return true;
            }
        }
        return false;
    }
};