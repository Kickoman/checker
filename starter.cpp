#include <csetjmp>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

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

int main() {
    auto result = execute("./echo", "Kek mek", "");
    std::cout << "Execution result: '" << result << "'" << std::endl;

    result = execute("./echo", "", "output to output");
    std::cout << "Execution result: " << result << std::endl;

    result = execute("ls", "", "");
    std::cout << "Result:\n" << result << std::endl;
    return 0;
}
