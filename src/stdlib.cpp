#include <algorithm>
#include <cstdio>
#include <exception>
#include <iostream>
#include <iterator>
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

void Environment::add_default_stdlib() {
    this->add("tostring", to_string);
    this->add("to_number", to_number);
    this->add("type", type);
    this->add("assert", assert_lua);
    this->add("next", next);
    this->add("select", select);
    this->add("print", print);
    this->add("discard_origin", discard_origin);
}

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

    return arg.to_string(ctx.call_location());
}

auto to_number(const CallContext& ctx) -> Value {
    auto number = ctx.arguments().get(0);
    auto base = ctx.arguments().get(1);

    return number.to_number(base, ctx.call_location());
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

auto discard_origin(const CallContext& ctx) -> Vallist {
    const Vallist& args = ctx.arguments();
    std::vector<Value> values;
    values.reserve(args.size());

    std::transform(args.begin(), args.end(), std::back_inserter(values), [](const Value& value) {
        return value.remove_origin();
    });

    return Vallist(values);
}

} // namespace minilua
