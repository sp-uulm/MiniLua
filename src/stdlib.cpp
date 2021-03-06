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
#include "internal_env.hpp"

namespace minilua {

auto force(const CallContext& ctx) -> CallResult {
    auto old_value = ctx.arguments().get(0);
    auto new_value = ctx.arguments().get(1);

    if (old_value.is_nil() || new_value.is_nil()) {
        throw std::runtime_error("requires two arguments (old_value and new_value)");
    }

    return CallResult(old_value.force(new_value));
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
    */

namespace details {

void add_stdlib(Table& table) {
    table.set("tostring", to_string);
    table.set("to_number", to_number);
    table.set("type", type);
    table.set("next", next);
    table.set("select", select);
    table.set("print", print);
    table.set("error", error);

    table.set("getmetatable", get_metatable);
    table.set("setmetatable", set_metatable);

    // non official lua stdlib items
    table.set("discard_origin", discard_origin);
    table.set("debug_print", debug_print);
}

} // namespace details

void error(const CallContext& ctx) {
    // TODO implement level (we need a proper call stack for that)
    auto message = ctx.arguments().get(0);
    throw std::runtime_error(std::get<String>(message.to_string()).value);
}

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

void debug_print(const CallContext& ctx) {
    auto& err = *ctx.environment().get_stderr();
    for (const auto& value : ctx.arguments()) {
        err << value << "\n";
    }
}

auto get_metatable(const CallContext& ctx) -> Value {
    auto arg = ctx.arguments().get(0);
    if (arg.is_table()) {
        auto metatable = std::get<Table>(arg).get_metatable();
        if (metatable.has_value()) {
            auto __metatable = metatable->get("__metatable");
            if (!__metatable.is_nil()) {
                return __metatable;
            } else {
                return metatable.value();
            }
        }
    }

    return Nil();
}

auto set_metatable(const CallContext& ctx) -> Value {
    auto table = std::get<Table>(ctx.expect_argument<Table>(0));
    const auto& arg2 = ctx.expect_argument<Nil, Table>(1);

    if (table.get_metatable().has_value() && !table.get_metatable()->get("__metatable").is_nil()) {
        throw std::runtime_error("cannot change a protected metatable");
    }

    if (arg2.is_nil()) {
        table.set_metatable(std::nullopt);
    } else if (arg2.is_table()) {
        auto metatable = std::get<Table>(arg2);
        table.set_metatable(metatable);
    }

    return table;
}

auto rawget(const CallContext& ctx) -> Value {
    const auto& table = std::get<Table>(ctx.expect_argument<Table>(0));
    const auto& arg2 = ctx.expect_argument<>(1);

    return table.get(arg2);
}

auto rawset(const CallContext& ctx) -> Value {
    auto table = std::get<Table>(ctx.expect_argument<Table>(0));
    const auto& key = ctx.expect_argument<>(1);
    const auto& value = ctx.expect_argument<>(1);

    table.set(key, value);

    return table;
}

} // namespace minilua
