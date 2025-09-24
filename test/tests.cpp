#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <libjdb/error.hpp>
#include <libjdb/process.hpp>
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
