#ifndef JDB_PROCESS_HPP
#define JDB_PROCESS_HPP

#include <cstdint>
#include <filesystem>
#include <libjdb/registers.hpp>
#include <memory>
#include <optional>
#include <sys/types.h>
#include <sys/user.h>

namespace jdb {

enum class process_state { stopped, running, exited, terminated };

struct stop_reason {
    stop_reason(int wait_status);

    process_state reason;
    std::uint8_t info;
};

class process {
  public:
    static std::unique_ptr<process> launch(std::filesystem::path path, bool debug = true,
                                           std::optional<int> stdout_replacement = std::nullopt);
    static std::unique_ptr<process> attach(pid_t pid);

    void resume();
    // /*?*/ wait_on_signal();
    pid_t pid() const { return pid_; }

    // delete the constructor and copy constructors so the client code is forced to use the static
    // members
    process() = delete;
    process(const process &) = delete;
    process &operator=(const process &) = delete;

    // create a publicly available destructor
    ~process();

    process_state state() const { return state_; }
    stop_reason wait_on_signal();

    registers &get_registers() { return *registers_; }
    const registers &get_registers() const { return *registers_; }

    void write_fprs(const user_fpregs_struct &fprs);
    void write_gprs(const user_regs_struct &gprs);

    void write_user_area(std::size_t offset, std::uint64_t data);

  private:
    // private constructor, so that the client can only create a member of the class by calling the
    // launch and attach public functions
    process(pid_t pid, bool terminate_on_end, bool is_attached)
        : pid_(pid), terminate_on_end_(terminate_on_end), is_attached_(is_attached),
          registers_(new registers(*this)) {}

    void read_all_registers();

    pid_t pid_ = 0;
    bool terminate_on_end_ = true;
    process_state state_ = process_state::stopped;
    bool is_attached_;
    std::unique_ptr<registers> registers_;
};

} // namespace jdb

#endif // !JDB
