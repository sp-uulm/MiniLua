#include <cstdio>
#include <exception>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "MiniLua/environment.hpp"
#include "MiniLua/stdlib.hpp"
#include "MiniLua/utils.hpp"

namespace minilua {

/**
Splits a string into two parts. the split happens at the character c which is not included in the
result.

Example:
split_string("123.456", '.') = (123, 456)
*/
// commented because not needed at the moment, maybe in the future. if not, delete it
/*static auto split_string(const std::string& s, char c) -> std::pair<std::string, std::string> {
    std::pair<std::string, std::string> result;
    std::stringstream split(s);
    std::string tmp;
    std::getline(split, tmp, c);
    result.first = tmp;
    std::getline(split, tmp, c);
    result.second = tmp;
    return result;
}
*/

auto to_string(const CallContext& ctx) -> Value {
    auto arg = ctx.arguments().get(0);
    return std::visit(
        overloaded{
            [](Bool b) -> Value { return b.value ? "true" : "false"; },
            [](Number n) -> Value { return n.to_literal(); },
            [](const String& s) -> Value { return s.value; },
            [](Table t) -> Value { // TODO: maybe improve the way to get the address.
                // at the moment it could be that every time you call it the
                // address has changed because of the change in the stack
                ostringstream get_address;
                get_address << &t;
                return get_address.str();
            },
            [](Function f) -> Value {
                ostringstream get_address;
                get_address << &f;
                return get_address.str();
            },
            [](Nil /*nil*/) -> Value { return "nil"; }
            // TODO: add to_string for metatables
        },
        arg.raw());
}

auto to_number(const CallContext& ctx) -> Value {
    auto number = ctx.arguments().get(0);
    auto base = ctx.arguments().get(1);

    return std::visit(
        overloaded{
            [](const String& number, Nil /*nil*/) -> Value {
                // Yes: parse number to double
                // No: return Nil
                std::regex pattern_number(R"(\s*-?\d+\.?\d*)");
                std::regex pattern_hex(R"(\s*-?0[xX][\dA-Fa-f]+\.?[\dA-Fa-f]*)");
                std::regex pattern_exp(R"(\s*-?\d+\.?\d*[eE]-?\d+)");

                if (std::regex_match(number.value, pattern_number) ||
                    std::regex_match(number.value, pattern_hex) ||
                    std::regex_match(number.value, pattern_exp)) {
                    return std::stod(number.value);
                } else {
                    return Nil();
                }
            },
            [](const String& number, Number base) -> Value {
                // Interval of base, with strings only numbers between base 2 and base 36 are
                // possible to show
                if (base < 2 || base > 36) { // NOLINT
                    throw std::runtime_error(
                        "base is to high or to low. base must be >= 2 and <= 36");
                } else {
                    std::regex pattern_number(R"(\s*-?[a-zA-Z0-9]+)");
                    // number must be interpreted as an integer numeral in that base
                    if (std::regex_match(number.value, pattern_number)) {
                        try {
                            return std::stoi(number.value, nullptr, base.value);
                        } catch (const std::invalid_argument& /*unused*/) {
                            // invalid base returns nil
                            return Nil();
                        }

                    } else {
                        return Nil();
                    }
                }
            },
            [](Number number, Nil /*unused*/) -> Value { return number; },
            [](auto /*a*/, auto /*b*/) -> Value { return Nil(); }},
        number.raw(), base.raw());
}

auto type(const CallContext& ctx) -> Value {
    auto v = ctx.arguments().get(0);

    return v.type();
}

auto assert_lua(const CallContext& ctx) -> Vallist {
    auto v = ctx.arguments().get(0);
    auto message = ctx.arguments().get(1);

    if (v) {
        return ctx.arguments();
    } else {
        // TODO: improve error behaviour
        throw std::runtime_error(
            message == Nil() ? std::string("assertion failed") : get<String>(message).value);
    }
}

auto next(const CallContext& ctx) -> Vallist {
    try {
        const auto& t = std::get<Table>(ctx.arguments().get(0));
        auto index = ctx.arguments().get(1);
        return t.next(index);
    } catch (std::bad_variant_access&) {
        auto a = ctx.arguments().get(0);
        throw std::runtime_error(
            "bad argument #1 to 'next' (table expected, got " + a.type() + ")");
    }
}

auto select(const CallContext& ctx) -> Vallist {
    auto index = ctx.arguments().get(0);
    std::vector<Value> args;

    for (auto a = ++ctx.arguments().begin(); a != ctx.arguments().end(); a++) {
        args.push_back(*a);
    }

    return std::visit(
        overloaded{
            [&args](Number n) -> Vallist {
                if (n == 0 || n < (int)args.size() * -1) {
                    throw std::runtime_error("bad argument #1 to 'select' (index out of range)");
                } else if (n > 0) {
                    std::vector<Value> returns;
                    int i = 1;
                    for (auto a = args.begin(); a != args.end(); a++, i++) {
                        if (i < n) {
                            continue;
                        } else {
                            returns.push_back(*a);
                        }
                    }

                    return Vallist(returns);
                } else {
                    std::vector<Value> returns;
                    int i = 1;

                    for (auto a = args.begin(); a != args.end(); a++, i++) {
                        //+ because n is negative so + becomes -
                        if (i <= (int)args.size() + n) {
                            continue;
                        } else {
                            returns.push_back(*a);
                        }
                    }

                    return Vallist(returns);
                }
            },
            [&args](String s) -> Vallist {
                if (std::move(s) == "#") {
                    int size = args.size();
                    return Vallist({Value(Number(size))});
                } else {
                    throw std::runtime_error(
                        "bad argument #1 to 'select' (number expected, got string)");
                }
            },
            [](auto a) -> Vallist {
                throw std::runtime_error(
                    "bad argument #1 to 'select' (number expected, got " + std::string(a.TYPE) +
                    ")");
            }},
        index.raw());
}

void print(const CallContext& ctx) {
    auto* const stdout = ctx.environment().get_stdout();
    std::string gap;

    for (const auto& arg : ctx.arguments()) {
        const Value v = to_string(ctx.make_new({arg}));

        if (v.is_string()) {
            *stdout << gap << std::get<String>(v).value;
            gap = "\t";
        }
    }
    *stdout << std::endl;
}
} // namespace minilua
