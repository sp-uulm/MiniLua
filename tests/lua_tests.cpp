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

static const std::regex source_change_regex("SOURCE_CHANGE (\\d+):(\\d+) (.*)");

/**
 * Expect a source change.
 *
 * ```lua
 * -- EXPECT SOURCE_CHANGE <row>:<col> <replacement>
 * -- EXPECT SOURCE_CHANGE 2:7 25
 * -- EXPECT SOURCE_CHANGE 2:7 "string"
 * ```
 */
struct SourceChangeTest : BaseTest {
    std::vector<ExpectedChange> expected_changes;

    SourceChangeTest() : BaseTest(source_change_regex) {}
    ~SourceChangeTest() override = default;

    void reset() override { this->expected_changes.clear(); }

    void collect_metadata(const std::string& str) override {
        std::smatch match;
        if (std::regex_match(str, match, this->regex)) {
            this->expected_changes.emplace_back(match);
        }
    }

    auto expect_source_changes() -> bool override { return !expected_changes.empty(); }

    void run(const minilua::EvalResult& result) override {
        CAPTURE(result);
        CAPTURE(expected_changes);

        // if we found source change tests we require to get source changes
        if (expected_changes.empty()) {
            CAPTURE(result.source_change);
            REQUIRE(!result.source_change.has_value());
        } else {
            REQUIRE(result.source_change.has_value());
            // check that the expected source changes are in the actual source
            // changes
            std::vector<minilua::SourceChange> actual_changes;
            result.source_change.value().visit_all(
                [&actual_changes](const auto& c) { actual_changes.push_back(c); });

            CAPTURE(actual_changes);

            for (const auto& expected_change : expected_changes) {
                bool is_in_actual_changes = any_of(actual_changes, expected_change);
                if (!is_in_actual_changes) {
                    CAPTURE(expected_change);
                    FAIL("Could not find expected_change in actual_changes");
                }
            }
        }
    }
};

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
