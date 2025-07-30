#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <editline/readline.h>
#include <iostream>
#include <libjdb/libjdb.hpp>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {
pid_t attach(int argc, const char** argv)
{
    pid_t pid = 0;
    // Passing a PID
    if (argc == 3 && argv[1] == std::string_view("-p")) {
        // In this branch, the program will attach to a running process
        pid = std::atoi(argv[2]);
        if (pid <= 0) {
            std::cerr << "Invalid pid\n";
            return -1;
        }
        if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) < 0) {
            std::perror("Could not attach");
            return -1;
        }
    } else {
        const char* program_path = argv[1];
        // fork is a syscall that splits the running process into two different processes
        if ((pid = fork()) < 0) {
            std::perror("Fork failed");
            return -1;
        }
        if (pid == 0) {
            // In child process
            // Execute debugee
            if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0) {
                std::perror("Tracing failed");
                return -1;
            }
            // exec* is a family of syscalls that replaces the currently executing program with a
            // new one. The l means that the arguments will be passed to the program individually,
            // as opposed to an array. The p tells exec to look for the given program name in the
            // PATH environment variable.
            if (execlp(program_path, program_path, nullptr) < 0) {
                std::perror("Exec failed");
                return -1;
            }
        }
    }
    return pid;
}

std::vector<std::string> split(std::string_view str, char delimiter)
{
    std::vector<std::string> out {};
    std::stringstream ss { std::string { str } };
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        out.push_back(item);
    }
    return out;
}
bool is_prefix(std::string_view str, std::string_view of)
{
    if (str.size() > of.size())
        return false;
    return std::equal(str.begin(), str.end(), of.begin());
}
// Wrapper for PTRACE_CONT
void resume(pid_t pid)
{
    if (ptrace(PTRACE_CONT, pid, nullptr, nullptr) < 0) {
        std::cerr << "Couldn't continue\n";
        std::exit(-1);
    }
}
// Wrapper for waitpid
void wait_on_signal(pid_t pid)
{
    int wait_status;
    int options = 0;
    if (waitpid(pid, &wait_status, options) < 0) {
        std::perror("waitpid failed");
        std::exit(-1);
    }
}
void handle_command(pid_t pid, std::string_view line)
{
    auto args = split(line, ' ');
    auto command = args[0];
    if (is_prefix(command, "continue")) {
        resume(pid);
        wait_on_signal(pid);
    } else {
        std::cerr << "Unknown command\n";
    }
}
}

int main(int argc, const char** argv)
{
    if (argc == 1) {
        std::cerr << "No arguments given\n";
        return -1;
    }
    pid_t pid = attach(argc, argv);

    int wait_status;
    int options = 0;
    if (waitpid(pid, &wait_status, options) < 0) {
        std::perror("waitpid failed");
    }
    char* line = nullptr;
    // readline creates a prompt anbd returns a char* with whatever the user wrote.
    // If it reads an EOF, it returns nullptr
    while ((line = readline("jdb> ")) != nullptr) {
        std::string line_str;

        // If the user doesn't write anything in the readline prompt, read the last command from the
        // libedit's history_list
        if (line == std::string_view("")) {
            // Freeing line's memory as soon as we figure out that we will not be using it, instead
            // of waiting for longer and risk missing it
            free(line);
            if (history_length > 0) {
                line_str = history_list()[history_length - 1]->line;
            }
        } else {
            line_str = line;
            add_history(line);
            free(line);
        }

        if (!line_str.empty()) {
            handle_command(pid, line_str);
        }
    }
}
