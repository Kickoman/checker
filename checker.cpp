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

struct Pipes {
    int pipefd[2];
    int &pipe_write = pipefd[1];
    int &pipe_read = pipefd[0];

    ~Pipes() {
        if (pipe_read_open) close(pipe_read);
        if (pipe_write_open) close(pipe_write);
    }

    bool open() {
        if (pipe(pipefd)) {
            return false;
        }
        pipe_write_open = true;
        pipe_read_open = true;
        return true;
    }
    void closeRead() {
        if (pipe_read_open) {
            close(pipe_read);
            pipe_read_open = false;
        }
    }
    void closeWrite() {
        if (pipe_write_open) {
            close(pipe_write);
            pipe_write_open = false;
        }
    }
private:
    bool pipe_write_open = false;
    bool pipe_read_open = false;
};

std::string execute(const std::string &command, const std::string &arguments, const std::string &input) {
    Pipes pin, pout;
    if (!pin.open() || !pout.open()) {
        std::cerr << "Failed to run command " << command << "!" << std::endl;
        std::cerr << strerror(errno) << std::endl;
        return {};
    }

    pid_t commandProcess = fork();
    if (commandProcess == 0) { // Child process
        pin.closeWrite();
        pout.closeRead();

        dup2(pin.pipe_read, 0);// Заменили стдин на чтение из входной трубы
        dup2(pout.pipe_write, 1); // А stdout на другую
        execlp(command.c_str(), command.c_str(), (!arguments.empty() ? arguments.c_str() : nullptr), nullptr);
        std::cerr << "Failed to execlp: " << strerror(errno) << std::endl;
    }

    if (commandProcess == -1) { // failed to start
        std::cerr << "Failed to fork: " << strerror(errno) << std::endl;
        return {};
    }

    pin.closeRead();
    pout.closeWrite();
    if (!input.empty()) {
        write(pin.pipe_write, input.c_str(), input.size());
    }
    pin.closeWrite();

    char buffer[1024];
    ssize_t bytesRead;
    std::string result;
    while ((bytesRead = read(pout.pipe_read, buffer, sizeof(buffer))) > 0) {
        result += std::string(buffer, bytesRead);
    }

    wait(nullptr);
    while (!result.empty() && result.back() == '\n') result.pop_back();
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
    const Path pythonExecutable = execute("which", "python3", "");
    std::cout << "'" << pythonExecutable << "'" << std::endl;
    // const Path pythonExecutable = execute_and_read("which python3");
    // std::cout << pythonExecutable << std::endl;
    const auto result = execute(pythonExecutable, "/home/knovikau/Documents/Program/cpp/checker/echo.py", "");
    std::cout << "Res: " << result << std::endl;
    // std::cout << execute(pythonExecutable, "", "/home/knovikau/Documents/Program/cpp/checker/echo.py");
    return 0;
}
