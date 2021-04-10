#include "lua_test_driver.hpp"

#include <MiniLua/MiniLua.hpp>
#include <catch2/catch.hpp>

#include <fstream>
#include <iostream>
#include <regex>
#include <string>

auto operator<<(std::ostream& o, const std::vector<minilua::SourceChange>& self) -> std::ostream& {
    o << "[";

    const char* sep = " ";
    for (const auto& change : self) {
        o << sep << change;
        sep = ", ";
    }

    return o << " ]";
}

static auto read_optional_file(const std::string& path) -> std::optional<std::string> {
    try {
        std::ifstream ifs;
        ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        ifs.open(path);
        return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

auto read_input_from_file(const std::string& path) -> std::string {
    return read_optional_file(path).value();
}

// struct ExpectedChange
ExpectedChange::ExpectedChange(const std::smatch& match)
    : row(stoul(match[1].str())), column(stoul(match[2].str())), replacement(match[3].str()) {}

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

// struct BaseTest
BaseTest::BaseTest(std::regex regex) : regex(std::move(regex)) {}
auto BaseTest::expect_source_changes() -> bool { return false; }

static std::vector<std::unique_ptr<BaseTest>> tests;
auto get_tests() -> std::vector<std::unique_ptr<BaseTest>>& { return tests; }

void register_test(BaseTest* test) { tests.push_back(std::unique_ptr<BaseTest>(test)); }

static auto change_extension(const std::string& path, const std::string& new_ext) -> std::string {
    auto last_dot = path.find_last_of('.');
    if (last_dot == std::string::npos) {
        throw std::runtime_error("Path has no dot: " + path);
    }

    return path.substr(0, last_dot + 1) + new_ext;
}

void test_file(const std::string& file) {
    std::string program = read_input_from_file(file);

    std::string stdin_path = change_extension(file, "in");
    std::string stdout_path = change_extension(file, "out");
    std::string stderr_path = change_extension(file, "err");

    std::optional<std::string> stdin_str = read_optional_file(stdin_path);
    std::optional<std::string> stdout_str = read_optional_file(stdout_path);
    std::optional<std::string> stderr_str = read_optional_file(stderr_path);

    // NOTE: We need to define streams here so they live long enough,
    // even if we don't always set them later

    // in stream to read from stdin (if any)
    std::istringstream stdin_stream;
    if (stdin_str.has_value()) {
        stdin_stream = std::istringstream(stdin_str.value());
    }
    // empty out/err streams to capture out/err
    std::ostringstream stdout_stream;
    std::ostringstream stderr_stream;

    auto expect_strings = find_expect_strings(program);

    // setup tests
    auto& tests = get_tests();
    for (auto& test : tests) {
        test->reset();
        for (const auto& str : expect_strings) {
            test->collect_metadata(str);
        }
    }

    // parse
    minilua::Interpreter interpreter;
    auto parse_result = interpreter.parse(program);
    CAPTURE(parse_result.errors);
    REQUIRE(parse_result);

    // setup stdin/out/err
    // if they are not provided we use the default
    if (stdin_str.has_value()) {
        interpreter.environment().set_stdin(&stdin_stream);
    }
    if (stdout_str.has_value()) {
        interpreter.environment().set_stdout(&stdout_stream);
    }
    if (stderr_str.has_value()) {
        interpreter.environment().set_stderr(&stderr_stream);
    }

    // evaluate
    try {
        auto result = interpreter.evaluate();

        // check
        for (auto& test : tests) {
            test->run(result);
        }
    } catch (const minilua::InterpreterException& e) {
        if (stderr_str.has_value()) {
            e.print_stacktrace(stderr_stream);
        }
    }

    // check stdout/err
    if (stdout_str.has_value() && *stdout_str != stdout_stream.str()) {
        INFO(*stdout_str);
        INFO(stdout_stream.str());
        FAIL("stdout did not match");
    }
    if (stderr_str.has_value() && *stderr_str != stderr_stream.str()) {
        INFO(*stderr_str);
        INFO(stderr_stream.str());
        FAIL("stderr did not match");
    }
}
