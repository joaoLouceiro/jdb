#ifndef JDB_PROCESS_HPP
#define JDB_PROCESS_HPP

#include <cstdint>
#include <filesystem>
#include <memory>
#include <sys/types.h>

namespace jdb {

enum class process_state { stopped, running, exited, terminated };

struct stop_reason {
    stop_reason(int wait_status);

    process_state reason;
    std::uint8_t info;
};

class process {
public:
    static std::unique_ptr<process> launch(std::filesystem::path path);
    static std::unique_ptr<process> attach(pid_t pid);

    void resume();
    // /*?*/ wait_on_signal();
    pid_t pid() const { return pid_; }

    // delete the constructor and copy constructors so the client code is forced to use the static
    // members
    process() = delete;
    process(const process&) = delete;
    process& operator=(const process&) = delete;

    // create a publicly available destructor
    ~process();

    process_state state() const { return state_; }
    stop_reason wait_on_signal();

private:
    pid_t pid_ = 0;
    bool terminate_on_end_ = true;
    process_state state_ = process_state::stopped;
    // private constructor, so that the client can only create a member of the class by calling the
    // launch and attach public functions
    process(pid_t pid, bool terminate_on_end)
        : pid_(pid)
        , terminate_on_end_(terminate_on_end)
    {
    }
};

} // namespace jdb

#endif // !JDB
