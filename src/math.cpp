#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include "MiniLua/math.hpp"
#include "MiniLua/source_change.hpp"
#include "MiniLua/stdlib.hpp"
#include "MiniLua/utils.hpp"
#include "MiniLua/values.hpp"

namespace minilua::math {

// functions to reduce the duplicate code because almost every math-function does the same thing
// except the function that is called to determine the new value
// gets 1 parameter
auto static math_helper(
    const CallContext& ctx, const std::function<double(double)>& function,
    const std::string& functionname, const std::string& arg_index = "1") -> Value {
    auto res = to_number(ctx);

    if (res != Nil()) {
        auto num = std::get<Number>(res);

        return Value(function(num.value));
    } else {
        auto x = ctx.arguments().get(0);
        throw std::runtime_error(
            "bad argument #" + arg_index + " to '" + functionname + "' (number expected, got " +
            x.type() + ")");
    }
}

// gets 2 parameter
auto static math_helper(
    const CallContext& ctx, const std::function<double(double, double)>& function,
    const std::string& functionname) -> Value {
    Vallist list = Vallist({ctx.arguments().get(0)});
    CallContext new_ctx = ctx.make_new(list);
    auto res1 = to_number(new_ctx);

    if (res1 != Nil()) {
        list = Vallist({ctx.arguments().get(1)});
        new_ctx = ctx.make_new(list);
        auto res2 = to_number(new_ctx);

        if (res2 != Nil()) {
            auto num1 = std::get<Number>(res1);
            auto num2 = std::get<Number>(res2);

            return Value(function(num1.value, num2.value));
        } else {
            auto y = ctx.arguments().get(0);
            throw std::runtime_error(
                "bad argument #2 to '" + functionname + "' (number expected, got " + y.type() +
                ")");
        }
    } else {
        auto x = ctx.arguments().get(0);
        throw std::runtime_error(
            "bad argument #1 to '" + functionname + "' (number expected, got " + x.type() + ")");
    }
}

// All math-functions can be called be a number or a string that is formated like a number.
// This information is not documented in the documentation. I just found that when I was testing the
// error behaviour for wrong input of the function in lua
// That is the reason why everywhere to_number is called.

auto abs(const CallContext& ctx) -> Value {
    return math_helper(ctx, static_cast<double (*)(double)>(&std::abs), "abs");
}

auto acos(const CallContext& ctx) -> Value {
    return math_helper(ctx, static_cast<double (*)(double)>(&std::acos), "acos");
}

auto asin(const CallContext& ctx) -> Value {
    return math_helper(ctx, static_cast<double (*)(double)>(&std::asin), "asin");
}

auto atan(const CallContext& ctx) -> Value {
    auto x = ctx.arguments().get(0);
    auto y = ctx.arguments().get(1);

    return std::visit(
        overloaded{
            [](Number x, Number y) { return Value(std::atan2(x.value, y.value)); },
            [ctx](Number x, String y) {
                Vallist list = Vallist({Value(std::move(y))});
                CallContext new_ctx = ctx.make_new(list);
                auto res = to_number(new_ctx);

                if (res != Nil()) {
                    auto num = get<Number>(res);

                    return Value(std::atan2(x.value, num.value));
                } else {
                    throw std::runtime_error(
                        "bad argument #2 to 'atan' (number expected, got string)");
                }
            },
            [](Number x, Nil /*unused*/) { return Value(std::atan2(x.value, 1)); },
            [](Number /*unused*/, auto y) -> Value {
                throw std::runtime_error(
                    "bad argument #2 to 'atan' (number expected, got " + std::string(y.TYPE) + ")");
            },
            [ctx](String x, Number y) {
                Vallist list = Vallist({Value(std::move(x))});
                CallContext new_ctx = ctx.make_new(list);
                auto res = to_number(new_ctx);

                if (res != Nil()) {
                    auto num = get<Number>(res);

                    return Value(std::atan2(num.value, y.value));
                } else {
                    throw std::runtime_error(
                        "bad argument #1 to 'atan' (number expected, got string)");
                }
            },
            [ctx](String x, String y) {
                Vallist list = Vallist({Value(std::move(x))});
                CallContext new_ctx = ctx.make_new(list);
                auto res1 = to_number(new_ctx);

                if (res1 != Nil()) {
                    list = Vallist({Value(std::move(y))});
                    new_ctx = ctx.make_new(list);
                    auto res2 = to_number(new_ctx);

                    if (res2 != Nil()) {
                        auto x1 = std::get<Number>(res1);
                        auto x2 = std::get<Number>(res2);

                        return Value(std::atan2(x1.value, x2.value));
                    } else {
                        throw std::runtime_error(
                            "bad argument #2 to 'atan' (number expected, got string)");
                    }
                } else {
                    throw std::runtime_error(
                        "bad argument #1 to 'atan' (number expected, got string)");
                }
            },
            [ctx](String x, Nil /*unused*/) {
                Vallist list = Vallist({Value(std::move(x))});
                CallContext new_ctx = ctx.make_new(list);
                auto res = to_number(new_ctx);

                if (res != Nil()) {
                    auto num = get<Number>(res);

                    return Value(std::atan2(num.value, 1));
                } else {
                    throw std::runtime_error(
                        "bad argument #1 to 'atan' (number expected, got string)");
                }
            },
            [ctx](String x, auto y) -> Value {
                Vallist list = Vallist({Value(std::move(x))});
                CallContext new_ctx = ctx.make_new(list);
                auto res = to_number(new_ctx);

                if (res != Nil()) {
                    throw std::runtime_error(
                        "bad argument #2 to 'atan' (number expected, got " + std::string(y.TYPE) +
                        ")");
                } else {
                    throw std::runtime_error(
                        "bad argument #1 to 'atan' (number expected, got string)");
                }
            },
            [](auto x, auto /*unused*/) -> Value {
                throw std::runtime_error(
                    "bad argument #1 to 'atan' (number expected, got " + std::string(x.TYPE) + ")");
            }},
        x.raw(), y.raw());
}

auto ceil(const CallContext& ctx) -> Value {
    return math_helper(ctx, static_cast<double (*)(double)>(&std::ceil), "ceil");
}

auto cos(const CallContext& ctx) -> Value {
    return math_helper(ctx, static_cast<double (*)(double)>(&std::cos), "cos");
}

auto deg(const CallContext& ctx) -> Value {
    return math_helper(
        ctx, [](double d) { return d * 180 / PI; }, "deg");
}

auto exp(const CallContext& ctx) -> Value {
    return math_helper(ctx, static_cast<double (*)(double)>(&std::exp), "exp");
}

auto floor(const CallContext& ctx) -> Value {
    return math_helper(ctx, static_cast<double (*)(double)>(&std::floor), "floor");
}

auto fmod(const CallContext& ctx) -> Value {
    // lua throws an error if and and y are 0 so i cant use the helper-function
    Vallist list = Vallist({ctx.arguments().get(0)});
    CallContext new_ctx = ctx.make_new(list);
    auto res1 = to_number(new_ctx);

    if (res1 != Nil()) {
        list = Vallist({ctx.arguments().get(1)});
        new_ctx = ctx.make_new(list);
        auto res2 = to_number(new_ctx);

        if (res2 != Nil()) {
            auto num1 = std::get<Number>(res1);
            auto num2 = std::get<Number>(res2);

            if (num1.value == 0 && num2.value == 0) {
                // case 0/0
                throw std::runtime_error("bad argument #2 to 'fmod' (zero)");
            } else {
                return Value(std::fmod(num1.value, num2.value));
            }
        } else {
            auto y = ctx.arguments().get(0);
            throw std::runtime_error(
                "bad argument #2 to 'fmod' (number expected, got " + y.type() + ")");
        }
    } else {
        auto x = ctx.arguments().get(0);
        throw std::runtime_error(
            "bad argument #1 to 'fmod' (number expected, got " + x.type() + ")");
    }
}

auto log(const CallContext& ctx) -> Value {
    auto x = ctx.arguments().get(0);
    auto base = ctx.arguments().get(1);

    return std::visit(
        overloaded{
            [ctx](auto /*unused*/, Nil /*unused*/) -> Value {
                return math_helper(ctx, static_cast<double (*)(double)>(&std::log), "log");
            },
            [ctx](auto a, auto b) -> Value {
                Vallist list({Value(a)});
                CallContext new_ctx = ctx.make_new(list);
                auto x = math_helper(new_ctx, static_cast<double (*)(double)>(&std::log), "log");
                list = Vallist({Value(b)});
                new_ctx = ctx.make_new(list);
                auto base =
                    math_helper(new_ctx, static_cast<double (*)(double)>(&std::log), "log", "2");
                Number x_n = std::get<Number>(x);
                Number base_n = std::get<Number>(base);
                return Value(x_n.value / base_n.value);
            }},
        x.raw(), base.raw());
}
} // namespace minilua::math