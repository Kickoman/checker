#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t pid;

    int &pipe_write = pipefd[1];
    int &pipe_read = pipefd[0];

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) { // Child process
        close(pipe_write); // Cloes write end of a pipe (Why?)
        dup2(pipe_read, STDIN_FILENO);

        execlp("./echo", "./echo", "kek lel", nullptr); // Replace "./echo" with your command
        perror("execlp"); // This line should not be reached if execlp succeeds
        return 1;
    } else { // Parent process
        close(pipefd[0]); // Close read end of the pipe

        // // Write to stdin of the child process
        std::string input = "somekek";
        write(pipefd[1], input.c_str(), input.size());

        close(pipefd[1]); // Close write end of the pipe

        // Read from stdout of the child process
        // char buffer[1024];
        // ssize_t bytesRead;
        // while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
        //     std::cout.write(buffer, bytesRead);
        // }

        wait(nullptr); // Wait for the child process to finish
    }

    return 0;
}
