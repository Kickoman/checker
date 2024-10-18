#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cerrno>

#include "kexec.h"

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

struct Test {
    Path inputData;
    Path outputData;
};

const std::string INPUT_FILE_SUFFIX = ".in";
const std::string OUTPUT_FILE_SUFFIX = ".out";

std::vector<Path> readTestsDirectory(const Path& path) {
    std::vector<Path> result;
    for (const auto &entry : std::filesystem::directory_iterator(path)) {
        if (endsWith(entry.path().string(), INPUT_FILE_SUFFIX) || endsWith(entry.path().string(), OUTPUT_FILE_SUFFIX)) {
            result.push_back(entry.path().string());
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

Path resolvePythonExecutable() {
#ifdef _WIN32
    const auto result = Kexec::execute("where.exe", "python");
    std::stringstream strean(result);
    std::string buffer;
    std::getline(strean, buffer, '\r');
    return buffer;
#else
    return Kexec::execute("which", "python3");
#endif
}


// Parameters: path to python code, path to folder with tests
// Each test is two files with the same name but different extension: in for input and out for output
int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Too few arguments" << std::endl;
        return 1;
    }
    const std::string testsDirectory = argv[1];
    const std::string pythonCode = argv[2];
    const Path pythonExecutable = resolvePythonExecutable();
    std::cout << "Using Python from: " << pythonExecutable << std::endl;
    std::cout << "Reading files from " << testsDirectory << std::endl;
    const auto files = readTestsDirectory(testsDirectory);
    std::cout << "Total test files: " << files.size() << std::endl;
    const auto tests = getTests(testsDirectory);
    std::cout << "Total tests: " << tests.size() << std::endl;

    for (const auto test : tests) {
        const auto testName = getFileNameWithoutExtension(test.inputData);
        std::cout << "Running test " << testName << ": ";

        std::ifstream inputData(test.inputData);
        std::ifstream outputData(test.outputData);

        std::stringstream inputBuffer;
        std::stringstream outputBuffer;
        inputBuffer << inputData.rdbuf();
        outputBuffer << outputData.rdbuf();

        std::string result = Kexec::execute(pythonExecutable, pythonCode, inputBuffer.str());
        if (result == outputBuffer.str())
            std::cout << "OK!" << std::endl;
        else
            std::cout << "FAILED! Expected " << outputBuffer.str() << ", but found " << result << std::endl;
    }
    return 0;
}
