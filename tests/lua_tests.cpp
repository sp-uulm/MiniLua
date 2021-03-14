#include <MiniLua/MiniLua.hpp>
#include <catch2/catch.hpp>

#include <algorithm>
#include <fnmatch.h>
#include <fstream>
#include <ftw.h>
#include <map>
#include <string>
#include <utility>

#include "lua_test_driver.hpp"

static const char* DIR = "../luaprograms/unit_tests/";

static std::vector<std::string> test_files;

static auto ftw_callback(const char* fpath, const struct stat* /*sb*/, int typeflag) -> int {
    if (typeflag == FTW_F) {
        if (fnmatch("*.lua", fpath, FNM_CASEFOLD) == 0) {
            test_files.emplace_back(fpath);
        }
    }

    // tell ftw to continue
    return 0;
}

TEST_CASE("lua file tests") {
    // collect files to test
    ftw(DIR, ftw_callback, 16); // NOLINT(readability-magic-numbers)

    register_test(new SourceChangeTest());
    // TODO add other tests

    // NOTE: expects to be run from build directory
    for (const auto& file : test_files) {
        DYNAMIC_SECTION("File: " << file) { test_file(file); }
    }
}
