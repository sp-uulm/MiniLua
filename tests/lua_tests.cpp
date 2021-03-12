#include "MiniLua/source_change.hpp"
#include <algorithm>
#include <catch2/catch.hpp>
#include <fnmatch.h>
#include <fstream>
#include <ftw.h>
#include <map>

#include <MiniLua/MiniLua.hpp>
#include <regex>
#include <string>
#include <utility>

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

auto operator<<(std::ostream& o, const std::vector<minilua::SourceChange>& self) -> std::ostream& {
    o << "[";

    const char* sep = " ";
    for (const auto& change : self) {
        o << sep << change;
        sep = ", ";
    }

    return o << " ]";
}

static auto read_input_from_file(const std::string& path) -> std::string {
    std::ifstream ifs;
    ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ifs.open(path);
    return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}

/**
 * Represents the following line in a lua file:
 *
 * ```lua
 * -- EXPECT SOURCE_CHANGE <row>:<column> <replacement>
 * ```
 */
struct ExpectedChange {
    size_t row;
    size_t column;
    std::string replacement;

    ExpectedChange(const std::smatch& match)
        : row(stoul(match[1].str())), column(stoul(match[2].str())), replacement(match[3].str()) {}
};

auto operator==(const ExpectedChange& expected, const minilua::SourceChange& actual) -> bool {
    return actual.range.start.line + 1 == expected.row &&
           actual.range.start.column + 1 == expected.column &&
           actual.replacement == expected.replacement;
}

auto operator<<(std::ostream& o, const ExpectedChange& self) -> std::ostream& {
    return o << "ExpectedChange{ " << self.row << ":" << self.column << " " << self.replacement
             << " }";
}

static const std::string COMMENT_PREFIX = "-- EXPECT ";

/**
 * Search for comments of the form:
 *
 * ```lua
 * -- EXPECT <something>
 * ```
 */
auto find_expect_strings(const std::string& source) -> std::vector<std::string> {
    std::vector<std::string> expect_strings;

    auto start_pos = 0;
    while (start_pos != std::string::npos) {
        start_pos = source.find(COMMENT_PREFIX, start_pos);
        if (start_pos == std::string::npos) {
            break;
        }

        start_pos = start_pos + COMMENT_PREFIX.length();

        auto end_pos = source.find('\n', start_pos);
        if (end_pos == std::string::npos) {
            end_pos = source.size() - 1;
        }

        std::string expect_str = source.substr(start_pos, end_pos - start_pos);
        expect_strings.push_back(expect_str);

        start_pos = end_pos;
    }

    return expect_strings;
}

template <typename Iterable, typename Item>
auto any_of(const Iterable& iterable, const Item& item) -> bool {
    return std::any_of(
        iterable.cbegin(), iterable.cend(), [item](const auto& actual) { return item == actual; });
}

struct BaseTest {
    std::regex regex;

    BaseTest(std::regex regex) : regex(std::move(regex)) {}
    virtual ~BaseTest() = default;

    virtual void collect_metadata(const std::string& str) = 0;

    virtual auto expect_source_changes() -> bool { return false; }

    virtual void run(const minilua::EvalResult& result) = 0;
};

static const std::regex source_change_regex("SOURCE_CHANGE (\\d+):(\\d+) (.*)");

struct SourceChangeTest : BaseTest {
    std::vector<ExpectedChange> expected_changes;

    SourceChangeTest() : BaseTest(source_change_regex) {}
    ~SourceChangeTest() override = default;

    void collect_metadata(const std::string& str) override {
        std::smatch match;
        if (std::regex_match(str, match, this->regex)) {
            this->expected_changes.emplace_back(match);
        }
    }

    auto expect_source_changes() -> bool override { return !expected_changes.empty(); }

    void run(const minilua::EvalResult& result) override {
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

auto get_tests() -> std::vector<std::unique_ptr<BaseTest>> {
    std::vector<std::unique_ptr<BaseTest>> tests;
    tests.push_back(std::make_unique<SourceChangeTest>());
    // TODO add other tests

    return tests;
}

void test_file(const std::string& file) {
    std::string program = read_input_from_file(file);

    auto expect_strings = find_expect_strings(program);

    // setup tests
    auto tests = get_tests();
    for (auto& test : tests) {
        for (const auto& str : expect_strings) {
            test->collect_metadata(str);
        }
    }

    // parse
    minilua::Interpreter interpreter;
    auto parse_result = interpreter.parse(program);
    CAPTURE(parse_result.errors);
    REQUIRE(parse_result);

    // evaluate
    auto result = interpreter.evaluate();

    // check
    for (auto& test : tests) {
        test->run(result);
    }
}

TEST_CASE("lua file tests") {
    // collect files to test
    ftw(DIR, ftw_callback, 16); // NOLINT(readability-magic-numbers)

    // NOTE: expects to be run from build directory
    for (const auto& file : test_files) {
        DYNAMIC_SECTION("File: " << file) { test_file(file); }
    }
}
