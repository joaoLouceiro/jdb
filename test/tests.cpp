#include <catch2/catch_test_macros.hpp>
#include <libjdb/error.hpp>
#include <libjdb/process.hpp>
#include <signal.h>
#include <sys/types.h>

using namespace jdb;

namespace {
bool process_exists(pid_t pid)
{
    auto ret = kill(pid, 0);
    return ret != -1 and errno != ESRCH;
}
}

TEST_CASE("process::launch success", "[process]")
{
    auto proc = process::launch("yes");
    REQUIRE(process_exists(proc->pid()));
}

TEST_CASE("process::launch no such program", "[process]")
{
    REQUIRE_THROWS_AS(process::launch("there_is_no_such_program_here"), error);
}
