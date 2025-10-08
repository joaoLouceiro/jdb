#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <libjdb/bit.hpp>
#include <libjdb/error.hpp>
#include <libjdb/pipe.hpp>
#include <libjdb/process.hpp>
#include <libjdb/register_info.hpp>
#include <signal.h>
#include <string>
#include <sys/types.h>

using namespace jdb;

namespace {
bool process_exists(pid_t pid) {
    auto ret = kill(pid, 0);
    return ret != -1 && errno != ESRCH;
}

char get_process_status(pid_t pid) {
    std::ifstream stat("/proc/" + std::to_string(pid) + "/stat");
    std::string data;
    std::getline(stat, data);
    auto index_of_last_parenthesis = data.rfind(')');
    auto index_of_status_indicator = index_of_last_parenthesis + 2;
    return data[index_of_status_indicator];
}
} // namespace

TEST_CASE("process::launch success", "[process]") {
    auto proc = process::launch("yes");
    REQUIRE(process_exists(proc->pid()));
}

TEST_CASE("process::launch no such program", "[process]") {
    REQUIRE_THROWS_AS(process::launch("there_is_no_such_program_here"), error);
}

// So, apparently, the CWD is the one we call ./tests from, so the process::launch path is not
// relative to the tests.cpp file. That means that, if we execute the tests from, say, /build
// (./test/tests), the program will look for /build/targets/run_endlessly, which doesn't exist,
// failing the test.
TEST_CASE("process:attach success", "[process]") {
    auto target = process::launch("test/targets/run_endlessly", false);
    auto proc = process::attach(target->pid());
    REQUIRE(get_process_status(target->pid()) == 't');
}

TEST_CASE("process:attach invalid PID", "[process]") {
    REQUIRE_THROWS_AS(process::attach(0), error);
}

TEST_CASE("process::resume success", "[process]") {
    {
        auto proc = process::launch("test/targets/run_endlessly");
        proc->resume();
        auto status = get_process_status(proc->pid());
        auto success = status == 'R' || status == 'S';
        REQUIRE(success);
    }
    {
        auto target = process::launch("test/targets/run_endlessly", false);
        auto proc = process::attach(target->pid());
        proc->resume();
        auto status = get_process_status(proc->pid());
        auto success = status == 'R' || status == 'S';
        REQUIRE(success);
    }
}

TEST_CASE("process::resume already terminated", "[process]") {
    auto proc = process::launch("test/targets/end_immediately");
    proc->resume();
    proc->wait_on_signal();
    REQUIRE_THROWS_AS(proc->resume(), error);
}

TEST_CASE("Write register works", "[register]") {
    // Our goal is to check, from within a running process, that we can affect the value of a
    // register.
    bool close_on_exec = false;
    jdb::pipe channel(close_on_exec);

    auto proc = process::launch("test/targets/reg_write", true, channel.get_write());
    channel.close_write();

    proc->resume();
    proc->wait_on_signal();

    // Because we are writing to rsi (where the syscall expects it's first parameter to live)
    // from the debugger's test, printf will send the value we just wrote to the standard out.
    auto &regs = proc->get_registers();
    regs.write_by_id(register_id::rsi, 0xcafecafe);

    proc->resume();
    proc->wait_on_signal();

    auto output = channel.read();
    REQUIRE(to_string_view(output) == "0xcafecafe");

    regs.write_by_id(register_id::mm0, 0xba5eba11);

    proc->resume();
    proc->wait_on_signal();

    output = channel.read();
    REQUIRE(to_string_view(output) == "0xba5eba11");

    regs.write_by_id(register_id::xmm0, 42.42);

    proc->resume();
    proc->wait_on_signal();

    output = channel.read();
    REQUIRE(to_string_view(output) == "42.42");

    // This test is a bit weirder. x87 registers work with their own sort of stack and set of
    // instructions. Getting the value onto the register itself is easy enough:
    regs.write_by_id(register_id::st0, 42.24l);
    // The FSW register is responsible for the Status Word. This tracks the the current size of the
    // FPU stack and reports errors like stack overflows, precision errors, or divisions by zero.
    // Bits 11 through 13 track the top of the stack, which starts at index 0 and goes down,
    // wrapping around to 7. To push the value to the stack, we set these bits to 7 (0b111).
    regs.write_by_id(register_id::fsw, std::uint16_t{0b0011100000000000});
    // The FPU Tag Word register tracks which of the st registers are valid, empty, or have a
    // special value like infinity or NaN. 0b00 is a valid register, 0b11 is invalid.
    regs.write_by_id(register_id::ftw, std::uint16_t{0b0011111111111111});

    proc->resume();
    proc->wait_on_signal();

    output = channel.read();
    REQUIRE(to_string_view(output) == "42.24");
}
