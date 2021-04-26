#include <catch2/catch.hpp>
#include <cstdio>
#include <fstream>
#include <ftw.h>
#include <iostream>
#include <linux/limits.h>
#include <string>
#include <type_traits>
#include <unistd.h>
#include <variant>

#include "MiniLua/interpreter.hpp"

static std::vector<int> open_fds;
static std::vector<std::string> open_files;
static auto ftw_callback(const char* fpath, const struct stat* sb, int typeflag) -> int {
    if (typeflag == FTW_F) {
        int num;

        sscanf(fpath, "/proc/self/fd/%d", &num);
        open_fds.push_back(num);

        // it not possible to get the link size for special files in e.g. /proc
        // so we just use PATH_MAX
        // this might truncate the filename but PATH_MAX should be big enough
        int bufsize = PATH_MAX + 1;
        char buf[bufsize];
        int nbytes = readlink(fpath, buf, bufsize);
        if (nbytes == -1) {
            throw std::runtime_error(std::string("failed to read symlink: ") + fpath);
        }
        buf[nbytes] = '\0';
        std::string link_target(buf);
        open_files.push_back(link_target);
    }
    return 0;
}

static auto read_proc_self_fd() -> std::vector<int> {
    open_fds.clear();
    open_files.clear();
    ftw("/proc/self/fd", ftw_callback, 1);
    return open_fds;
}

TEST_CASE("io all files are closed when the interpreter quits") {
    read_proc_self_fd();
    auto pre_run_open_fds = open_fds;
    auto pre_run_open_files = open_files;

    minilua::Interpreter interpreter;
    interpreter.parse(R"(io.open("/tmp/test.txt"))");
    auto result = interpreter.evaluate();
    REQUIRE(result.value.is_nil());

    read_proc_self_fd();
    auto post_run_open_fds = open_fds;
    auto post_run_open_files = open_files;

    REQUIRE_THAT(post_run_open_fds, Catch::Matchers::UnorderedEquals(pre_run_open_fds));
    REQUIRE_THAT(post_run_open_files, Catch::Matchers::UnorderedEquals(pre_run_open_files));
}
