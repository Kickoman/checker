#include "kexec.h"

// For close and pipes
// TODO: Add windows-compatible implementation
#include <unistd.h>
// For wait
// TODO: Add windows-compatible implementation
#include <sys/wait.h>

#include <fmt/core.h>

namespace Kexec {

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


StringType execute(const StringType& command, const StringType& arguments, const StringType& input) {
    Pipes pin, pout;
    if (!pin.open() || !pout.open()) {
        throw Kexeption(fmt::format(
            "Failed to run command '{}'! Error: {}", command, strerror(errno)
        ));
    }

    pid_t commandProcess = fork();
    if (commandProcess == 0) { // Child process
        pin.closeWrite();
        pout.closeRead();

        dup2(pin.pipe_read, 0);
        dup2(pout.pipe_write, 1);
        execlp(command.c_str(), command.c_str(), (!arguments.empty() ? arguments.c_str() : nullptr), nullptr);
        throw Kexeption(fmt::format(
            "Failed to execlp: {}", strerror(errno)
        ));
    }

    if (commandProcess == -1) { // failed to start
        throw Kexeption(fmt::format(
            "Failed to fork: {}", strerror(errno)
        ));
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

}
