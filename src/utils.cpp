#include <algorithm>
#include <sstream>

#include "MiniLua/utils.hpp"

namespace minilua {

const char ascii_0 = 48;
const char ascii_A = 65;

auto to_string_with_base(int number, int base) -> std::string {
    if (base < 2 || base >= 36) {
        throw std::runtime_error("base has to be between 2 and 35");
    }

    std::stringstream ss;

    auto print_digit = [&ss](int digit) {
        if (digit >= 10) {                    // NOLINT
            ss << char(digit + ascii_A - 10); // NOLINT
        } else {
            ss << char(digit + ascii_0);
        }
    };

    bool negative = false;
    if (number < 0) {
        negative = true;
        number = -number;
    }

    while (number >= base) {
        auto rest = number % base;
        print_digit(rest);
        number = number / base;
    }
    if (number != 0) {
        print_digit(number);
    }

    if (negative) {
        ss << '-';
    }

    std::string str = ss.str();
    std::reverse(str.begin(), str.end());

    return str;
}

auto string_starts_with(const std::string& str, char ch) -> bool {
    return !str.empty() && str[0] == ch;
}

} // namespace minilua
