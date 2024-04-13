#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <algorithm>

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
    return 0;
}
