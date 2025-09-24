#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <libjdb/error.hpp>
#include <libjdb/pipe.hpp>
#include <libjdb/process.hpp>
#include <memory>
#include <string>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
void exit_with_perror(jdb::pipe &channel, std::string const &prefix) {
    auto message = prefix + ": " + std::strerror(errno);
    channel.write(reinterpret_cast<std::byte *>(message.data()), message.size());
    exit(-1);
}
} // namespace

std::unique_ptr<jdb::process> jdb::process::launch(std::filesystem::path path, bool debug) {
    pipe channel(true);
    pid_t pid;
    // fork is a syscall that splits the running process into two different processes
    if ((pid = fork()) < 0) {
        error::send_errno("fork failed");
    }
    if (pid == 0) {
        // Because the child process will not be reading from the pipe, we can close it immediately
        channel.close_read();
        // fork returns 0 to the child process
        // Execute debugee
        if (debug && ptrace(PTRACE_TRACEME, 0, nullptr, nullptr)) {
            exit_with_perror(channel, "Tracing failed");
        }
        // exec* is a family of syscalls that replaces the currently executing program with a
        // new one. The l means that the arguments will be passed to the program individually,
        // as opposed to an array. The p tells exec to look for the given program name in the
        // PATH environment variable.
        if (execlp(path.c_str(), path.c_str(), nullptr) < 0) {
            exit_with_perror(channel, "exec failed");
        }
    }
    channel.close_write();
    auto data = channel.read();
    channel.close_read();
    // If any data has been written to the pipe, then an error has been thrown by the child.
    if (data.size() > 0) {
        waitpid(pid, nullptr, 0);
        auto chars = reinterpret_cast<char *>(data.data());
        error::send(std::string(chars, chars + data.size()));
    }
    std::unique_ptr<process> proc(
        new process(pid, /*terminate_on_end=*/true, /*is_attached=*/debug));
    if (debug) {
        proc->wait_on_signal();
    }
    return proc;
}

std::unique_ptr<jdb::process> jdb::process::attach(pid_t pid) {
    if (pid == 0) {
        error::send("Invalid PID");
    }
    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) < 0) {
        error::send_errno("Could not attach");
    }

    std::unique_ptr<process> proc(
        new process(pid, /*terminate_on_end=*/false, /*is_attached=*/true));
    proc->wait_on_signal();

    return proc;
}

jdb::process::~process() {
    if (pid_ != 0) {
        int status;
        if (is_attached_) {
            if (state_ == process_state::running) {
                kill(pid_, SIGSTOP);
                waitpid(pid_, &status, 0);
            }
            ptrace(PTRACE_DETACH, pid_, nullptr, nullptr);
            kill(pid_, SIGCONT);
        }

        if (terminate_on_end_) {
            kill(pid_, SIGKILL);
            waitpid(pid_, &status, 0);
        }
    }
}

// Wrapper for PTRACE_CONT
void jdb::process::resume() {
    if (ptrace(PTRACE_CONT, pid_, nullptr, nullptr) < 0) {
        error::send_errno("Could not resume");
    }
    state_ = process_state::running;
}

jdb::stop_reason::stop_reason(int wait_status) {
    if (WIFEXITED(wait_status)) {
        reason = process_state::exited;
        info = WEXITSTATUS(wait_status);
    } else if (WIFSIGNALED(wait_status)) {
        reason = process_state::terminated;
        info = WTERMSIG(wait_status);
    } else if (WIFSTOPPED(wait_status)) {
        reason = process_state::stopped;
        info = WSTOPSIG(wait_status);
    }
}

// Wrapper for waitpid
jdb::stop_reason jdb::process::wait_on_signal() {
    int wait_status;
    int options = 0;
    if (waitpid(pid_, &wait_status, options) < 0) {
        error::send_errno("waitpid failed");
    }
    stop_reason reason(wait_status);
    state_ = reason.reason;
    return reason;
}
