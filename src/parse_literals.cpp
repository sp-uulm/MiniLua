#include "MiniLua/values.hpp"

#include <iterator>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

namespace minilua {

static std::string pattern_decimal = R"((\s*-?\s*\d+\.?\d*))";
static std::string pattern_hex = R"((\s*-?\s*0[xX][\dA-Fa-f]+\.?[\dA-Fa-f]*([pP][-+]?\d+)?))";
static std::string pattern_hex_2 = R"((\s*-?\s*0[xX]\.[\dA-Fa-f]*([pP][-+]?\d+)?))";
static std::string pattern_scientific_notation = R"((\s*-?\s*\d+\.?\d*([eE][-+]?\d+)?))";
static std::string pattern_scientific_notation_2 = R"((\s*-?\s*\.\d*([eE][-+]?\d+)?))";
static std::regex to_number_general_pattern(
    pattern_decimal + "|" + pattern_hex + "|" + pattern_hex_2 + "|" + pattern_scientific_notation +
        "|" + pattern_scientific_notation_2,
    std::regex::ECMAScript | std::regex::optimize);
static std::regex to_number_int_pattern(R"(\s*-?\s*([0-9]+|(0[xX][a-fA-F0-9]+)))");

auto parse_number_literal(const std::string& str) -> Value {
    // if the string matches the expected format we parse it otherwise return nil
    if (std::regex_match(str, to_number_int_pattern)) {
        // parse as int and check if whole input is consumed
        try {
            char* last_pos;

            auto str_copy = str;
            str_copy.erase(remove_if(str_copy.begin(), str_copy.end(), isspace), str_copy.end());
            // we need to parse as unsigned long to retain the same behaviour as lua
            auto value = static_cast<Number::Int>(std::strtoul(str_copy.c_str(), &last_pos, 0));
            // NOTE this sets `errno` to `ERANGE` if the result is out of range but
            // we ignore this to automatically truncate the result
            if (std::distance(str_copy.c_str(), static_cast<const char*>(last_pos)) !=
                str_copy.length()) {
                throw std::runtime_error(
                    "Could not completely parse integer literal. This is a bug.");
            }

            return Value(value);
        } catch (const std::out_of_range& e) {
            throw std::runtime_error("integer is out of range");
        }
    } else if (std::regex_match(str, to_number_general_pattern)) {
        // parse as double and check if whole input is consumed
        try {
            auto str_copy = str;
            str_copy.erase(remove_if(str_copy.begin(), str_copy.end(), isspace), str_copy.end());
            auto value = std::stod(str_copy);
            // NOTE if the value is out of range HUGE_VAL is returned
            // if (false) {
            //    throw std::runtime_error(
            //        "Could not completely parse float literal. This is a bug.");
            //}
            return Value(value);
        } catch (const std::out_of_range& e) {
            throw std::runtime_error("float is out of range");
        } catch (const std::invalid_argument& e) {
            throw std::runtime_error("No conversion to float possible");
        }
    } else {
        return Nil();
    }
}

// helper functions

/**
 * Helper function to check if the string literal "delimiters" are correct and get their length.
 *
 * Delimiters are either `"` or `'` on both sides of the string or `[=[` on the start and `]=]` on
 * the end. But the number of `=` has to match (can also be no `=`).
 */
static auto check_and_get_trim(const std::string& s) -> size_t {
    char first_char = s.at(0);
    if (first_char == '"') {
        if (s.at(s.length() - 1) != '"') {
            throw std::runtime_error("invalid string literal (start and end marker don't match)");
        }

        return 1;
    } else if (first_char == '\'') {
        if (s.at(s.length() - 1) != '\'') {
            throw std::runtime_error("invalid string literal (start and end marker don't match)");
        }

        return 1;
    } else if (first_char == '[') {
        size_t trim = 2;
        while (s.at(trim - 1) == '=') {
            trim++;
        }

        if (s.at(trim - 1) != '[') {
            throw std::runtime_error(
                "invalid string literal (start of long bracket string needs to follow the form [=[ with any number of =)");
        }

        bool valid_end = true;

        if (s.at(s.length() - trim) != ']') {
            valid_end = false;
        }
        if (trim > 2) {
            for (size_t i = s.length() - trim + 1; i < s.length() - 1; ++i) {
                if (s[i] != '=') {
                    valid_end = false;
                    break;
                }
            }
        }
        if (s.at(s.length() - 1) != ']') {
            valid_end = false;
        }

        if (!valid_end) {
            throw std::runtime_error("invalid string literal "
                                     "(end of long bracket string needs to follow the form ]=] "
                                     "with the same number of = as the start of the string)");
        }

        return trim;
    } else {
        throw std::runtime_error("invalid string literal (should start with \", \' or [)");
    }
}

static auto escape_string(const std::string& s, size_t trim) -> std::string {
    std::stringstream ss;

#define LUA_STR_PARSE_ESCAPE(c, replacement)                                                       \
    case c:                                                                                        \
        ss << replacement;                                                                         \
        i++;                                                                                       \
        break;

    // ignore the leading and trailing `"`s (which are guaranteed by the grammar)
    for (size_t i = trim; i < s.length() - trim; ++i) {
        if (s[i] == '\\') {
            auto c = s.at(i + 1);
            switch (c) {
                LUA_STR_PARSE_ESCAPE('a', "\a")
                LUA_STR_PARSE_ESCAPE('b', "\b")
                LUA_STR_PARSE_ESCAPE('f', "\f")
                LUA_STR_PARSE_ESCAPE('n', "\n")
                LUA_STR_PARSE_ESCAPE('r', "\r")
                LUA_STR_PARSE_ESCAPE('t', "\t")
                LUA_STR_PARSE_ESCAPE('v', "\v")
                LUA_STR_PARSE_ESCAPE('\\', "\\")
                LUA_STR_PARSE_ESCAPE('"', "\"")
                LUA_STR_PARSE_ESCAPE('\'', "\'")
                LUA_STR_PARSE_ESCAPE('\n', "\n")
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                char value = c - '0';
                int count = 1;
                i++;
                c = s.at(i + 1);
                while ('0' <= c && c <= '9' && count < 3) {
                    value = value * 10 + c - '0'; // NOLINT(readability-magic-numbers)
                    i++;
                    count++;
                    c = s.at(i + 1);
                }
                ss << value;
                break;
            }
            case 'x':
                // TODO
            default:
                std::stringstream error;
                error << "invalid escape sequence near '\\" << s[i + 1] << "'";
                throw std::runtime_error(error.str());
            }
        } else {
            ss << s[i];
        }
    }

    return ss.str();
}

auto parse_string_literal(const std::string& s) -> Value {
    // relevant documentation:
    // https://www.lua.org/manual/5.1/manual.html#2.8

    size_t trim = check_and_get_trim(s);

    if (trim > 1) {
        // for long bracket string:
        // - ignore first leading newline
        // - no escaping
        size_t start_trim = trim;
        if (s[start_trim] == '\n') {
            start_trim++;
        }

        return s.substr(start_trim, s.length() - start_trim - trim);
    }

    return escape_string(s, trim);
}

} // namespace minilua
