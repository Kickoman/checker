#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>
#include <unistd.h>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cerrno>
#include <sys/wait.h>

using Path = std::string;

// Utils
bool endsWith(const std::string &str, const std::string &suffix) {
    if (str.size() >= suffix.size()) {
        return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }
    return false;
}

std::string getFileName(const Path& path, const std::string& delimiter = "/") {
    return path.substr(path.find_last_of(delimiter) + 1);
}

std::string getFileNameWithoutExtension(const Path &path, const std::string& delimiter = "/") {
    const auto filename = getFileName(path, delimiter);
    return filename.substr(0, filename.find_last_of('.'));
}

std::string execute_and_read(const std::string &command) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(
        popen(command.c_str(), "r"),
        pclose
    );
    if (!pipe) {
        std::cerr << "Failed to run command " << command << "!" << std::endl;
        std::cerr << strerror(errno) << std::endl;
        return {};
    }

    // Read from stdout
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result.substr(0, result.find_last_of('\n'));
}

std::string execute(const std::string &command, const std::string &input, const std::string &arguments = "") {
    int pipefd[2];
    pid_t pid;

    if (pipe(pipefd) == -1) {
        std::cerr << "Failed to pipe: " << strerror(errno) << std::endl;
        return {};
    }

    pid = fork();
    if (pid == -1) {
        std::cerr << "Failed to fork: " << strerror(errno) << std::endl;
        return {};
    }

    std::string result;
    if (pid == 0) { // Child process
        close(pipefd[1]);
        std::cout << "Arguments: " << arguments << std::endl;
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            std::cerr << "Failed to dup: " << strerror(errno) << std::endl;
            exit(1);
        }
        execlp(command.c_str(), arguments.c_str(), nullptr);
        std::cerr << "Failed to execlp: " << strerror(errno) << std::endl;
    } else { // Parent process
        close(pipefd[0]); // Close read end of the pipe

        // Write to stdin of the child process
        write(pipefd[1], input.c_str(), input.size());

        close(pipefd[1]); // Close write end of the pipe

        // Read from stdout of the child process
        char buffer[1024];
        ssize_t bytesRead;
        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            result += std::string(buffer, bytesRead);
        }

        wait(nullptr); // Wait for the child process to finish
    }
    return result;
}

struct Test {
    Path inputData;
    Path outputData;
};

const std::string INPUT_FILE_SUFFIX = ".in";
const std::string OUTPUT_FILE_SUFFIX = ".out";

std::vector<Path> readTestsDirectory(const Path& path) {
    std::vector<Path> result;
    for (const auto &entry : std::filesystem::directory_iterator(path)) {
        if (endsWith(entry.path(), INPUT_FILE_SUFFIX) || endsWith(entry.path(), OUTPUT_FILE_SUFFIX)) {
            result.push_back(entry.path());
        }
    }
    return result;
}

std::vector<Test> getTests(const Path& path) {
    auto files = readTestsDirectory(path);
    if (files.size() % 2 != 0) {
        std::cerr << "Found inconsistence: odd number of test files." << std::endl;
    }

    std::sort(files.begin(), files.end());

    std::vector<Test> tests;
    for (size_t i = 0; i < files.size(); ++i) {
        const auto testNumber = tests.size();
        const auto &firstFile = files[i];
        const auto testName = getFileNameWithoutExtension(firstFile);

        if (i + 1 == files.size()) {
            std::cerr << "Found inconsistence: only one file for test " << testName << ". Skipping." << std::endl;
            continue;
        }

        const auto &secondFile = files[i + 1];
        if (getFileNameWithoutExtension(secondFile) != testName) {
            std::cerr << "Found inconsistence: only one file for test " <<  testName << ". Skipping." << std::endl;
            continue;
        }

        const auto &inputFile = endsWith(firstFile, INPUT_FILE_SUFFIX) ? firstFile : secondFile;
        const auto &outputFile = endsWith(secondFile, OUTPUT_FILE_SUFFIX) ? secondFile : firstFile;
        tests.push_back({inputFile, outputFile});
        ++i;
    }
    return tests;
}


// Parameters: path to python code, path to folder with tests
// Each test is two files with the same name but different extension: in for input and out for output
int main(int argc, char **argv) {
    /*
    const std::string testsDirectory = argv[1];
    const auto files = readTestsDirectory(testsDirectory);
    std::cout << "Read files from " << testsDirectory << std::endl;
    std::cout << "Total count: " << files.size() << std::endl;
    for (const auto & file : files)
        std::cout << "\tFile: " << getFileName(file) << " " << getFileNameWithoutExtension(file) << "\n";

    const auto tests = getTests(testsDirectory);
    std::cout << "Total found tests: " << tests.size() << std::endl;
    for (const auto & test : tests)
        std::cout << "\tTest " << getFileNameWithoutExtension(test.inputData) << "\n";

    const auto result = execute("./echo", "watafak_mazafak");
    std::cout << "Result: " << result << std::endl;
*/
    const Path pythonExecutable = execute_and_read("which python3");
    std::cout << pythonExecutable << std::endl;
    std::cout << execute(pythonExecutable, "", "/home/knovikau/Documents/Program/cpp/checker/echo.py");
    return 0;
}
