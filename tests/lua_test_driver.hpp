#ifndef TESTS_LUA_TEST_DRIVER_HPP
#define TESTS_LUA_TEST_DRIVER_HPP

#include <MiniLua/MiniLua.hpp>
#include <catch2/catch.hpp>

#include <iostream>
#include <regex>

auto operator<<(std::ostream& o, const std::vector<minilua::SourceChange>& self) -> std::ostream&;

auto read_input_from_file(const std::string& path) -> std::string;

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

    ExpectedChange(const std::smatch& match);
};

auto operator==(const ExpectedChange& expected, const minilua::SourceChange& actual) -> bool;

auto operator<<(std::ostream& o, const ExpectedChange& self) -> std::ostream&;

/**
 * Search for comments of the form:
 *
 * ```lua
 * -- EXPECT <something>
 * ```
 */
auto find_expect_strings(const std::string& source) -> std::vector<std::string>;

template <typename Iterable, typename Item>
auto any_of(const Iterable& iterable, const Item& item) -> bool {
    return std::any_of(
        iterable.cbegin(), iterable.cend(), [item](const auto& actual) { return item == actual; });
}

/**
 * Base class for "-- EXPECT" test cases in lua files.
 *
 * Usage:
 *
 * - setup
 *   - first call reset
 *   - then call collect_metadata on every substring of a comment
 *     (the part after the "-- EXPECT")
 * - check
 *   - then call run using the result of evaluating the program
 */
struct BaseTest {
    std::regex regex;

    BaseTest(std::regex regex);
    virtual ~BaseTest() = default;

    virtual void reset() = 0;

    virtual void collect_metadata(const std::string& str) = 0;

    virtual void run(const minilua::EvalResult& result) = 0;

    /**
     * Returns false by default.
     *
     * Only override if you (always or conditionally) expect source changes.
     */
    virtual auto expect_source_changes() -> bool;
};

auto get_tests() -> std::vector<std::unique_ptr<BaseTest>>&;
void register_test(BaseTest* test);

void test_file(const std::string& file);

#endif
