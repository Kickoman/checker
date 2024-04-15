#include <iostream>
#include <cstdio>
#include <cstring>
#include <array>
#include <memory>

std::string execute_and_read(const std::string &command) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(
        popen("./capitalizer", "r"),
        pclose
    );
    if (!pipe) {
        std::cerr << "Failed to run command " << command << "!" << std::endl;
        std::cerr << strerror(errno) << std::endl;
        return {};
    }

    fputs("privet", pipe.get());
    fputs(static_cast<char>(-1), pipe.get());

    // Read from stdout
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result.substr(0, result.find_last_of('\n'));
}
std::string capitalizeWord(const std::string& word) {
    std::array<char, 128> buffer;
    std::string result;

    // Open a pipe to execute the "capitalize" program
    FILE* pipe = popen("./capitalizer", "r");
    if (!pipe) {
        std::cerr << "Error: Unable to open pipe to capitalize program." << std::endl;
        return "";
    }

    // Write the word to the standard input of the "capitalize" program
    fprintf(pipe, "%s\n", word.c_str());
    fflush(pipe);

    // Read the capitalized word from the standard output of the "capitalize" program
    char c;
    while (fread(&c, sizeof(char), 1, pipe) == 1 && c != '\n') {
        result += c;
    }

    // Close the pipe
    pclose(pipe);

    return result;
}

int main() {
    std::string word;
    std::cout << "Enter a word: ";
    std::cin >> word;

    std::string capitalizedWord = execute_and_read("");
    std::cout << "Capitalized word: " << capitalizedWord << std::endl;

    return 0;
}
