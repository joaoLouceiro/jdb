#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <editline/readline.h>
#include <iostream>
#include <libjdb/error.hpp>
#include <libjdb/process.hpp>
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
std::unique_ptr<jdb::process> attach(int argc, const char **argv) {
    pid_t pid = 0;
    // Passing a PID
    if (argc == 3 && argv[1] == std::string_view("-p")) {
        // In this branch, the program will attach to a running process
        pid = std::atoi(argv[2]);
        return jdb::process::attach(pid);
    } else {
        const char *program_path = argv[1];
        return jdb::process::launch(program_path);
    }
}

std::vector<std::string> split(std::string_view str, char delimiter) {
    std::vector<std::string> out{};
    std::stringstream ss{std::string{str}};
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        out.push_back(item);
    }
    return out;
}

bool is_prefix(std::string_view str, std::string_view of) {
    if (str.size() > of.size())
        return false;
    return std::equal(str.begin(), str.end(), of.begin());
}

void print_stop_reason(const jdb::process &process, jdb::stop_reason reason) {
    std::cout << "Process " << process.pid() << ' ';

    switch (reason.reason) {
    case jdb::process_state::exited:
        std::cout << "exited with status " << static_cast<int>(reason.info);
        break;
    case jdb::process_state::terminated:
        std::cout << "terminated with signal " << sigabbrev_np(reason.info);
        break;
    case jdb::process_state::stopped:
        std::cout << "stopped with signal " << sigabbrev_np(reason.info);
        break;
    }

    std::cout << std::endl;
}
void handle_command(std::unique_ptr<jdb::process> &process, std::string_view line) {
    auto args = split(line, ' ');
    auto command = args[0];
    if (is_prefix(command, "continue")) {
        process->resume();
        auto reason = process->wait_on_signal();
        print_stop_reason(*process, reason);
    } else {
        std::cerr << "Unknown command\n";
    }
}
} // namespace

void main_loop(std::unique_ptr<jdb::process> &process) {
    char *line = nullptr;
    // readline creates a prompt and returns a char* with whatever the user wrote.
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
            handle_command(process, line_str);
        }
    }
}

int main(int argc, const char **argv) {
    if (argc == 1) {
        std::cerr << "No arguments given\n";
        return -1;
    }
    try {
        auto process = attach(argc, argv);
        main_loop(process);
    } catch (const jdb::error &err) {
        std::cout << err.what() << '\n';
    }
}
