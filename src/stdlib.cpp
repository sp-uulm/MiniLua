#include <exception>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "MiniLua/stdlib.hpp"
#include "MiniLua/utils.hpp"
#include "MiniLua/values.hpp"

namespace minilua {

/**
Splits a string into two parts. the split happens at the character c which is not included in the
result.

Example:
split_string("123.456", '.') = (123, 456)
*/
static auto split_string(const std::string& s, char c) -> std::pair<std::string, std::string> {
    std::pair<std::string, std::string> result;
    std::stringstream split(s);
    std::string tmp;
    std::getline(split, tmp, c);
    result.first = tmp;
    std::getline(split, tmp, c);
    result.second = tmp;
    return result;
}

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
                std::regex pattern_number(R"(\s*\d+\.?\d*)");
                std::regex pattern_hex(R"(\s*0[xX][\dA-Fa-f]+\.?[\dA-Fa-f]*)");
                std::regex pattern_exp(R"(\s*\d+\.?\d*[eE]-?\d+)");

                if (std::regex_match(number.value, pattern_number) ||
                    std::regex_match(number.value, pattern_hex) ||
                    std::regex_match(number.value, pattern_exp)) {
                    return std::stod(number.value);
                } else {
                    return Nil();
                }
            },
            [](const String& number, Number base) -> Value {
                // match again with pattern, but this time with 1 .
                // Interval of base, with strings only numbers bezween base 2 and base 36 are
                // possible to show
                if (base < 2 || base > 36) { // NOLINT
                    throw std::runtime_error("base is to high. base must be >= 2 and <= 36");
                } else {
                    std::regex pattern_number(R"(\s*\d+\.\d*)");
                    std::regex pattern_hex(R"(\s*0[xX][\dA-Za-z]+\.[\dA-Za-z]*)");
                    std::regex pattern_exp(R"(\s*\d+\.\d*[eE]-?\d+)");
                    // parse number to double
                    if (std::regex_match(number.value, pattern_number) ||
                        std::regex_match(number.value, pattern_hex)) {
                        auto parts = split_string(number.value, '.');
                        int precomma = std::stoi(parts.first, nullptr, base.value);
                        int postcomma = std::stoi(parts.second, nullptr, base.value);
                        return precomma + postcomma * std::pow(base.value, parts.second.size());
                    } else if (std::regex_match(number.value, pattern_exp)) {
                        auto number_exp = split_string(number.value, 'e');
                        int exp = std::stoi(number_exp.second);
                        auto parts = split_string(number_exp.first, '.');
                        int precomma = std::stoi(parts.first, nullptr, base.value);
                        int postcomma = std::stoi(parts.second, nullptr, base.value);
                        double number_res = precomma + precomma +
                                            postcomma * std::pow(base.value, parts.second.size());
                        return number_res * std::pow(base.value, exp);
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

    if (bool(v)) {
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
                if (n == -1) {
                    return Vallist({*(--args.end())});
                } else if (n == 0) {
                    throw std::runtime_error("bad argument #1 to 'select' (index out of range)");
                } else {
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
} // namespace minilua