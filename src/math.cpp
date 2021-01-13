#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include "MiniLua/math.hpp"
#include "MiniLua/stdlib.hpp"
#include "MiniLua/utils.hpp"
#include "MiniLua/values.hpp"

namespace minilua::math {

// All math-functions can be called be a number or a string that is formated like a number.
// This information is not documented in the documentation. I just found that when I was testing the
// error behaviour for wrong input of the function in lua
// That is the reason why everywhere to_number is called.

auto abs(const CallContext& ctx) -> Value {
    auto x = ctx.arguments().get(0);
    auto res = minilua::to_number(ctx);

    if (res != Nil()) {
        auto num = get<Number>(res);

        return Value(std::abs(num.value));
    } else {
        throw std::runtime_error(
            "bad argument #1 to 'abs' (number expected, got " + x.type() + ")");
    }
}

auto acos(const CallContext& ctx) -> Value {
    auto x = ctx.arguments().get(0);
    auto res = minilua::to_number(ctx);

    if (res != Nil()) {
        auto num = get<Number>(res);

        if (num.value >= -1 && num.value <= 1) {
            return Value(std::acos(num.value));
        } else {
            return Value("nan");
        }
    } else {
        throw std::runtime_error(
            "bad argument #1 to 'acos' (number expected, got " + x.type() + ")");
    }
}

auto asin(const CallContext& ctx) -> Value {
    auto x = ctx.arguments().get(0);
    auto res = minilua::to_number(ctx);

    if (res != Nil()) {
        auto num = get<Number>(res);

        if (num.value >= -1 && num.value <= 1) {
            return Value(std::asin(num.value));
        } else {
            return Value("nan");
        }
    } else {
        throw std::runtime_error(
            "bad argument #1 to 'asin' (number expected, got " + x.type() + ")");
    }
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
    auto x = ctx.arguments().get(0);
    auto res = minilua::to_number(ctx);

    if (res != Nil()) {
        auto num = get<Number>(res);

        return Value(std::ceil(num.value));
    } else {
        throw std::runtime_error(
            "bad argument #1 to 'ceil' (number expected, got " + x.type() + ")");
    }
}

auto cos(const CallContext& ctx) -> Value {
    auto x = ctx.arguments().get(0);
    auto res = minilua::to_number(ctx);

    if (res != Nil()) {
        auto num = get<Number>(res);

        return Value(std::cos(num.value));
    } else {
        throw std::runtime_error(
            "bad argument #1 to 'cos' (number expected, got " + x.type() + ")");
    }
}

auto deg(const CallContext& ctx) -> Value {
    auto x = ctx.arguments().get(0);
    auto res = minilua::to_number(ctx);

    if (res != Nil()) {
        auto num = get<Number>(res);
        double deg = num.value * 180 / boost::math::constants::pi<double>();

        return Value(deg);
    } else {
        throw std::runtime_error(
            "bad argument #1 to 'deg' (number expected, got " + x.type() + ")");
    }
}

auto exp(const CallContext& ctx) -> Value {
    auto x = ctx.arguments().get(0);
    auto res = minilua::to_number(ctx);

    if (res != Nil()) {
        auto num = get<Number>(res);

        return Value(std::exp(num.value));
    } else {
        throw std::runtime_error(
            "bad argument #1 to 'exp' (number expected, got " + x.type() + ")");
    }
}
} // namespace minilua::math