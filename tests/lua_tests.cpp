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

static auto read_input_from_file(const std::string& path) -> std::string {
    std::ifstream ifs;
    ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ifs.open(path);
    return std::string(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
}

struct ExpectedChange {
    int row;
    int column;
    std::string replacement;
};

static auto operator<<(std::ostream& o, const ExpectedChange& self) -> std::ostream& {
    return o << "ExpectedChange{ .row = " << self.row << ", .column = " << self.column
             << ", .replacement = " << self.replacement << " }";
}

static auto operator<<(std::ostream& o, const std::vector<minilua::SourceChange>& self)
    -> std::ostream& {
    o << "[";

    const char* sep = " ";
    for (const auto& change : self) {
        o << sep << change;
        sep = ", ";
    }

    return o << " ]";
}

TEST_CASE("unit_tests lua files") {
    ftw(DIR, ftw_callback, 16); // NOLINT(readability-magic-numbers)

    // NOTE: expects to be run from build directory
    for (const auto& file : test_files) {
        DYNAMIC_SECTION("File: " << file) {
            std::string program = read_input_from_file(file);

            std::vector<ExpectedChange> expected_changes;

            auto start_pos = 0;
            while (start_pos != std::string::npos) {
                start_pos = program.find("-- EXPECT ", start_pos);
                if (start_pos == std::string::npos) {
                    break;
                }

                start_pos = start_pos + 10;

                auto end_pos = program.find('\n', start_pos);
                if (end_pos == std::string::npos) {
                    end_pos = program.size() - 1;
                }

                std::string expect_str = program.substr(start_pos, end_pos - start_pos);

                start_pos = end_pos;

                std::regex expect_regex("SOURCE_CHANGE (\\d+):(\\d+) (.*)");
                std::smatch match;

                if (std::regex_match(expect_str, match, expect_regex)) {
                    ExpectedChange e{
                        .row = std::stoi(match[1].str()),
                        .column = std::stoi(match[2].str()),
                        .replacement = match[3].str(),
                    };

                    expected_changes.push_back(e);
                }
            }

            minilua::Interpreter interpreter;
            REQUIRE(interpreter.parse(program));
            auto result = interpreter.evaluate();

            if (expected_changes.empty()) {
                REQUIRE(!result.source_change.has_value());
            } else {
                std::vector<minilua::SourceChange> actual_changes;
                result.source_change.value().visit_all(
                    [&actual_changes](const auto& c) { actual_changes.push_back(c); });

                INFO(&actual_changes);

                for (const auto& expected_change : expected_changes) {
                    if (!std::any_of(
                            actual_changes.begin(), actual_changes.end(),
                            [&expected_change](const auto& change) {
                                return change.range.start.line + 1 == expected_change.row &&
                                       change.range.start.column + 1 == expected_change.column &&
                                       change.replacement == expected_change.replacement;
                            })) {
                        FAIL("Could not find expected change " << expected_change);
                    }
                }
            }
        }
    }
}
