#include "kexec.h"
#include "types.h"

// TODO: Add windows-compatible implementation
// For close and pipes
#ifdef linux
#include <unistd.h>
// For wait
#include <sys/wait.h>
#endif // linux

#ifdef WIN32
#include <windows.h>
#endif
// TODO: Add windows-compatible implementation

#include <fmt/core.h>
#include <sstream>

#include <iostream>
#include <cassert>

namespace Kexec {

#ifdef linux

    struct Pipes {
        int pipefd[2];
        int& pipe_write = pipefd[1];
        int& pipe_read = pipefd[0];

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

    std::vector<StringType> split(const StringType& s, const StringType::value_type delimiter) {
        std::vector<StringType> tokens;
        std::stringstream stream(s);
        std::string buffer;
        while (std::getline(stream, buffer, delimiter)) {
            tokens.push_back(buffer);
        }
        return tokens;
    }


    StringType execute(const StringType& command, const StringType& arguments, const StringType& input) {
        Pipes pin, pout;
        if (!pin.open() || !pout.open()) {
            throw KException(fmt::format(
                "Failed to run command '{}'! Error: {}", command, strerror(errno)
            ));
        }

        pid_t commandProcess = fork();
        if (commandProcess == 0) { // Child process
            pin.closeWrite();
            pout.closeRead();

            // TODO: Fix this later, when all consumers are ready for new format
            const auto splittedArguments = split(arguments, ' ');

            std::vector<const char*> carguments(splittedArguments.size() + 2);
            carguments[0] = command.c_str();
            for (size_t i = 0; i < splittedArguments.size(); ++i) {
                carguments[i + 1] = splittedArguments[i].c_str();
            }
            carguments[splittedArguments.size() + 1] = nullptr;

            dup2(pin.pipe_read, 0);
            dup2(pout.pipe_write, 1);

            // TODO: DEAL WITH CONSTS WHEN FIXED ISSUE ABOVE
            execvp(command.c_str(), const_cast<char* const*>(carguments.data()));
            throw KException(fmt::format(
                "Failed to execlp: {}", strerror(errno)
            ));
        }

        if (commandProcess == -1) { // failed to start
            throw KException(fmt::format(
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

#else

struct Pipes {
    HANDLE readHandle = nullptr;
    HANDLE writeHandle = nullptr;

    ~Pipes() {
        if (readHandle) CloseHandle(readHandle);
        if (writeHandle) CloseHandle(writeHandle);
    }

    bool open() {
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE; // Pipe handles are inherited by child process
        saAttr.lpSecurityDescriptor = nullptr;

        if (!CreatePipe(&readHandle, &writeHandle, &saAttr, 0)) {
            return false;
        }

        return true;
    }

    void closeRead() {
        if (readHandle) {
            CloseHandle(readHandle);
            readHandle = nullptr;
        }
    }

    void closeWrite() {
        if (writeHandle) {
            CloseHandle(writeHandle);
            writeHandle = nullptr;
        }
    }
};

StringType execute(const StringType& command, const StringType& arguments, const StringType& input) {
    Pipes pin, pout;
    if (!pin.open() || !pout.open()) {
        throw KException(fmt::format("Failed to create pipes for '{}' with args '{}' and input '{}'", command, arguments, input));
    }

    SetHandleInformation(pin.readHandle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    SetHandleInformation(pin.writeHandle, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(pout.readHandle, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(pout.writeHandle, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);


    std::string commandLine = command + " " + arguments;
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = pout.writeHandle;
    si.hStdOutput = pout.writeHandle;
    si.hStdInput = pin.readHandle;
    si.dwFlags |= STARTF_USESTDHANDLES;

    if (!CreateProcessA(
        nullptr,                        // No module name (use command line)
        const_cast<char*>(commandLine.c_str()), // Command line
        nullptr,                        // Process handle not inheritable
        nullptr,                        // Thread handle not inheritable
        TRUE,                           // Set handle inheritance to TRUE
        0,                              // No creation flags
        nullptr,                        // Use parent's environment block
        nullptr,                        // Use parent's starting directory 
        &si,                            // Pointer to STARTUPINFO structure
        &pi                             // Pointer to PROCESS_INFORMATION structure
    )) {
        throw KException(fmt::format("Failed to create process of '{}'", commandLine));
    }

    pin.closeRead();
    pout.closeWrite();

    if (!input.empty()) {
        DWORD written = 0;
        if (!WriteFile(pin.writeHandle, input.c_str(), static_cast<DWORD>(input.size()), &written, nullptr)) {
            DWORD error = GetLastError();
            throw KException(fmt::format("Failed to write to process input. Error: {}", error));
        }

        FlushFileBuffers(pin.writeHandle);
    }
    pin.closeWrite();

    char buffer[1024]{};
    DWORD bytesRead;
    std::string result;
    while (true) {
        BOOL success = ReadFile(pout.readHandle, buffer, sizeof(buffer), &bytesRead, nullptr);
        if (!success || bytesRead == 0) {
            DWORD exitCode;
            if (!GetExitCodeProcess(pi.hProcess, &exitCode) || exitCode != STILL_ACTIVE) {
                break; // Process has finished, exit the loop.
            }
            Sleep(10);
        }
        else {
            result.append(buffer, bytesRead);
        }
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) result.pop_back();

    return result;
}

#endif // linux

}
