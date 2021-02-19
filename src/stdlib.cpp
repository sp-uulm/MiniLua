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
#include "internal_env.hpp"

namespace minilua {

namespace details {

void add_stdlib(Table& table) {
    table.set("tostring", to_string);
    table.set("to_number", to_number);
    table.set("type", type);
    table.set("next", next);
    table.set("select", select);
    table.set("print", print);
    table.set("error", error);
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
} // namespace minilua
